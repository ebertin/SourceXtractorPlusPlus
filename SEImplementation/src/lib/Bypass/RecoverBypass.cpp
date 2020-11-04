/** Copyright © 2019 Université de Genève, LMU Munich - Faculty of Physics, IAP-CNRS/Sorbonne Université
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 3.0 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <fstream>
#include "SEImplementation/Property/SourceId.h"
#include "SEImplementation/Property/PixelCoordinateList.h"
#include "SEImplementation/Plugin/GroupInfo/GroupInfo.h"
#include "SEImplementation/Plugin/SourceIDs/SourceID.h"
#include "SEImplementation/Bypass/RecoverBypass.h"

namespace SourceXtractor {

RecoverBypass::RecoverBypass(const std::string& path,
                             const std::shared_ptr<DetectionImageFrame::ImageFilter>& filter,
                             const std::shared_ptr<SourceFactory>& source_factory,
                             const std::shared_ptr<SourceGroupFactory>& group_factory)
  : Segmentation(nullptr), Deblending({}),
    m_path{path}, m_filter{filter},
    m_source_factory{source_factory}, m_group_factory{group_factory} {
}

void RecoverBypass::handleMessage(const std::shared_ptr<SourceGroupInterface>&) {
  throw Elements::Exception() << "RecoverBypass can not consume messages!";
}

void RecoverBypass::processFrame(std::shared_ptr<DetectionImageFrame> frame) const {
  std::ifstream in(m_path);
  if (!in) {
    throw Elements::Exception() << "Failed to open the bypass file " << m_path;
  }
  if (m_filter) {
    frame->setFilter(m_filter);
  }

  int source_count = 0;

  while (in) {
    unsigned group_id, n_sources;
    in >> group_id >> n_sources;
    if (!in)
      break;

    auto group = m_group_factory->createSourceGroup();
    group->setProperty<GroupInfo>(group_id);

    while (n_sources) {
      auto source = m_source_factory->createSource();

      int det_id, src_id;
      size_t n_pixel;
      in >> det_id >> src_id >> n_pixel;
      source->setProperty<SourceID>(src_id, det_id);
      source->setProperty<SourceId>(det_id);

      std::vector<PixelCoordinate> coord_list(n_pixel);
      for (auto& coord : coord_list) {
        in >> coord.m_x >> coord.m_y;
      }
      source->setProperty<PixelCoordinateList>(coord_list);
      source->setProperty<DetectionFrame>(frame);

      Observable<std::shared_ptr<SourceInterface>>::notifyObservers(source);

      group->addSource(source);
      --n_sources;
      ++source_count;
    }

    Observable<std::shared_ptr<SourceGroupInterface>>::notifyObservers(group);
  }

  auto height = frame->getOriginalImage()->getHeight();
  Observable<SegmentationProgress>::notifyObservers({height, height});
}

} // end of namespace SourceXtractor
