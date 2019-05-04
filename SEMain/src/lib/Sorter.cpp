#include <SEImplementation/Plugin/SourceIDs/SourceID.h>
#include <algorithm>
#include "SEMain/Sorter.h"

namespace SExtractor {

static unsigned int extractSourceId(const SourceInterface &i) {
  return i.getProperty<SourceID>().getId();
}

Sorter::Sorter(): m_output_next{1} {
}

void Sorter::handleMessage(const std::shared_ptr<SourceGroupInterface> &message) {
  std::vector<unsigned int> source_ids(message->size());
  std::transform(message->cbegin(), message->cend(), source_ids.begin(), extractSourceId);
  std::sort(source_ids.begin(), source_ids.end());

  auto first_source_id = source_ids.front();
  m_output_buffer.emplace(first_source_id, message);

  while (!m_output_buffer.empty() && m_output_buffer.begin()->first == m_output_next) {
    auto &next_group = m_output_buffer.begin()->second;
    m_output_next += next_group->size();
    notifyObservers(next_group);
    m_output_buffer.erase(m_output_buffer.begin());
  }
}


} // end SExtractor
