#include "SEMain/ProgressBar.h"
#include "ModelFitting/ModelFitting/utils.h"

#include <sys/ioctl.h>
#include <iostream>
#include <boost/date_time.hpp>
#include <signal.h>
#include <boost/thread.hpp>
#include <ncurses.h>
#include <term.h>
#include <mutex>

namespace SExtractor {

static struct sigaction sigterm, sigwinch;
static std::map<int, struct sigaction> prev_signal;
static std::mutex terminal_mutex;

// Forward declaration
static void handleTerminatingSignal(int s);

static void handleTerminalResize(int s);

class WinStream : public std::streambuf {
private:
  WINDOW *m_window;

public:
  WinStream(WINDOW *win) : m_window(win) {
  }

  std::streambuf::int_type overflow(std::streambuf::int_type c) override {
    if (c != traits_type::eof()) {
      waddch(m_window, c);
    }
    return c;
  }

  int sync() override {
    std::lock_guard<std::mutex> lock(terminal_mutex);
    wrefresh(m_window);
    return 0;
  }
};

/**
 * Wrap the terminal. There is one single logical instance, so everything is static.
 * This used to be inside ProgressBar, but it turns out that the destructor was not being called
 * when unwinding the stack. (Probably because it was a singleton?)
 * ProgressBar can *not* be a singleton if we isolate this data and code here.
 */
class Terminal {
private:
  int m_progress_lines, m_progress_y;
  std::streambuf *m_cerr_original_rdbuf;
  std::unique_ptr<std::streambuf> m_cerr_sink;

public:
  WINDOW *m_progress_window, *m_log_window;
  int m_value_position, m_bar_width;

  void restore() {
    // Exit ncurses
    delwin(m_progress_window);
    delwin(m_log_window);
    endwin();

    // Restore cerr
    std::cerr.rdbuf(m_cerr_original_rdbuf);

    // Restore signal handlers
    ::sigaction(SIGINT, &prev_signal[SIGINT], nullptr);
    ::sigaction(SIGTERM, &prev_signal[SIGTERM], nullptr);
    ::sigaction(SIGABRT, &prev_signal[SIGABRT], nullptr);
    ::sigaction(SIGSEGV, &prev_signal[SIGSEGV], nullptr);
    ::sigaction(SIGHUP, &prev_signal[SIGHUP], nullptr);
    ::sigaction(SIGWINCH, nullptr, nullptr);
  }

  void prepare(int gress_lines, int max_label_width) {
    // C++ guarantees that a static object is initialized once, and in a thread-safe manner
    // We do it here so this setup only happens *if* the progress bar is used
    struct Helper {
      Helper() {
        ::memset(&sigterm, 0, sizeof(sigterm));

        sigterm.sa_handler = &handleTerminatingSignal;
        ::sigaction(SIGINT, &sigterm, &prev_signal[SIGINT]);
        ::sigaction(SIGTERM, &sigterm, &prev_signal[SIGTERM]);
        ::sigaction(SIGABRT, &sigterm, &prev_signal[SIGABRT]);
        ::sigaction(SIGSEGV, &sigterm, &prev_signal[SIGSEGV]);
        ::sigaction(SIGHUP, &sigterm, &prev_signal[SIGHUP]);

        sigwinch.sa_handler = &handleTerminalResize;
        ::sigaction(SIGWINCH, &sigwinch, nullptr);
      }
    } _helper;

    // Enter ncurses
    SCREEN *term = newterm(nullptr, stderr, stdin);
    set_term(term);
    curs_set(0);

    // Setup colors
    use_default_colors();
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_GREEN);
    init_pair(2, COLOR_WHITE, COLOR_BLACK);

    // Compute window and progress bar sizes
    m_progress_lines = gress_lines;
    m_value_position = max_label_width + 3;
    initializeSizes();

    // Create windows
    m_progress_window = newwin(m_progress_lines, COLS, m_progress_y, 0);
    m_log_window = newwin(m_progress_y, COLS, 0, 0);
    scrollok(m_log_window, TRUE);

    // Capture stderr to intercept Elements logging and the like
    m_cerr_original_rdbuf = std::cerr.rdbuf();
    m_cerr_sink.reset(new WinStream(m_log_window));
    std::cerr.rdbuf(m_cerr_sink.get());
  }

  void resize() {
    std::lock_guard<std::mutex> lock(terminal_mutex);
    endwin();
    refresh();

    initializeSizes();
    mvwin(m_progress_window, m_progress_y, 0);
    wresize(m_progress_window, m_progress_lines + 1, COLS);
    wresize(m_log_window, m_progress_y, COLS);
    refresh();
  }

private:
  void initializeSizes() {
    // Reserve the bottom side for the progress report
    m_progress_y = LINES - m_progress_lines;
    // Precompute the width of the bars (remember brackets!)
    m_bar_width = COLS - m_value_position - 2;
  }
};

// Declare static members
static Terminal terminal;

/**
 * Intercept several terminating signals so the terminal style can be restored
 */
static void handleTerminatingSignal(int s) {
  terminal.restore();

  // Call the previous handler
  ::sigaction(s, &prev_signal[s], nullptr);
  ::raise(s);
}

/**
 * Intercept terminal window resize
 */
static void handleTerminalResize(int) {
  terminal.resize();
}


ProgressBar::ProgressBar()
  : m_started{boost::posix_time::second_clock::local_time()} {
}

ProgressBar::~ProgressBar() {
  if (m_progress_thread) {
    m_progress_thread->interrupt();
    m_progress_thread->join();
  }
  terminal.restore();
}

bool ProgressBar::isTerminalCapable() {
  setupterm(nullptr, fileno(stderr), nullptr);
  return isatty(fileno(stderr));
}

void ProgressBar::handleMessage(const std::map<std::string, std::pair<int, int>>& info) {
  this->ProgressReporter::handleMessage(info);

  // On first call, prepare and spawn the progress report block
  if (!m_progress_thread) {
    // Put always the value at the same distance: length of the longest attribute + space + (space|[) (starts at 1!)
    size_t max_attr_len = 0;

    for (auto& e : info) {
      max_attr_len = std::max(max_attr_len, e.first.size());
    }

    // Prepare the terminal
    terminal.prepare(info.size() + 1, max_attr_len);

    // Start printer
    m_progress_thread = make_unique<boost::thread>(ProgressBar::printThread, this);
  }
}

void ProgressBar::handleMessage(const bool& done) {
  this->ProgressReporter::handleMessage(done);
  if (m_progress_thread)
    m_progress_thread->join();
  terminal.restore();
}

void ProgressBar::printThread(void *d) {
  auto self = static_cast<ProgressBar *>(d);
  while (!self->m_done && !boost::this_thread::interruption_requested()) {
    auto now = boost::posix_time::second_clock::local_time();
    auto elapsed = now - self->m_started;

    // Restore position to the beginning
    wclear(terminal.m_progress_window);
    wmove(terminal.m_progress_window, 0, 0);

    // Now, print the actual progress
    int line = 0;
    for (auto entry : self->m_progress_info) {
      wmove(terminal.m_progress_window, line, 0);
      // When there is no total, show an absolute count
      if (entry.second.second < 0) {
        wattron(terminal.m_progress_window, A_BOLD);
        waddstr(terminal.m_progress_window, entry.first.c_str());
        wattroff(terminal.m_progress_window, A_BOLD);
        wmove(terminal.m_progress_window, line, terminal.m_value_position);
        wprintw(terminal.m_progress_window, "%d", entry.second.first);
      }
        // Otherwise, report progress as a bar
      else {
        float ratio = 0;
        if (entry.second.second > 0) {
          ratio = entry.second.first / float(entry.second.second);
          if (ratio > 1)
            ratio = 1.;
        }

        // Build the report string
        std::ostringstream bar;
        bar << entry.second.first << " / " << entry.second.second
            << " (" << std::fixed << std::setprecision(2) << ratio * 100. << "%)";

        // Attach as many spaces as needed to fill the screen width, minus brackets
        bar << std::string(terminal.m_bar_width - bar.str().size(), ' ');

        // Print label
        wattron(terminal.m_progress_window, A_BOLD);
        waddstr(terminal.m_progress_window, entry.first.c_str());
        wattroff(terminal.m_progress_window, A_BOLD);
        mvwaddch(
          terminal.m_progress_window,
          line, terminal.m_value_position,
          '['
        );

        // Completed
        auto bar_content = bar.str();
        int completed = bar_content.size() * ratio;

        wattron(terminal.m_progress_window, COLOR_PAIR(1));
        waddstr(terminal.m_progress_window, bar_content.substr(0, completed).c_str());
        wattroff(terminal.m_progress_window, COLOR_PAIR(1));

        // Rest
        wattron(terminal.m_progress_window, COLOR_PAIR(2));
        waddstr(terminal.m_progress_window, bar_content.substr(completed).c_str());
        wattroff(terminal.m_progress_window, COLOR_PAIR(2));

        // Closing bracket
        waddch(terminal.m_progress_window, ']');
      }
      ++line;
    }
    // Elapsed time
    wattron(terminal.m_progress_window, A_BOLD);
    mvwaddstr(terminal.m_progress_window, line, 0, "Elapsed");
    wattroff(terminal.m_progress_window, A_BOLD);
    mvwaddstr(
      terminal.m_progress_window,
      line, terminal.m_value_position + 1,
      (boost::lexical_cast<std::string>(elapsed)).c_str()
    );

    // Flush
    {
      std::lock_guard<std::mutex> lock(terminal_mutex);
      wrefresh(terminal.m_progress_window);
    }

    try {
      boost::this_thread::sleep(boost::posix_time::seconds(1));
    }
    catch (const boost::thread_interrupted&) {
      break;
    }
  }
}

} // end SExtractor
