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

#include "SEImplementation/Plugin/SourceIDs/SourceID.h"
#include "SEImplementation/Plugin/GroupInfo/GroupInfo.h"
#include "SEImplementation/Property/PixelCoordinateList.h"
#include "SEImplementation/Bypass/StoreBypass.h"

namespace SourceXtractor {

StoreBypass::StoreBypass(const std::string& path) : m_outstream{path, std::ios_base::trunc} {
}

void StoreBypass::handleMessage(const std::shared_ptr<SourceGroupInterface>& message) {
  const auto& group_info = message->getProperty<GroupInfo>();
  m_outstream << group_info.getGroupId() << " " << message->size() << std::endl;
  for (const auto& src : *message) {
    const auto& src_id = src.getProperty<SourceID>();
    const auto& pixels = src.getProperty<PixelCoordinateList>();

    const auto& coord_list = pixels.getCoordinateList();
    m_outstream << '\t' << src_id.getDetectionId() << ' ' << src_id.getId() << ' '
                << coord_list.size() << std::endl << '\t';

    for (const auto& coord : coord_list) {
      m_outstream << coord.m_x << ' ' << coord.m_y << ' ';
    }
    m_outstream << std::endl;
  }
  m_outstream.flush();
}

} // end of namespace SourceXtractor
