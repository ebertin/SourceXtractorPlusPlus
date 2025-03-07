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
/*
 * MeasurementFrameRectangleTask.cpp
 *
 *  Created on: Sep 24, 2018
 *      Author: Alejandro Alvarez Ayllon
 */

#include <SEImplementation/Plugin/MeasurementFrameCoordinates/MeasurementFrameCoordinates.h>
#include <SEImplementation/Plugin/MeasurementFrameInfo/MeasurementFrameInfo.h>
#include <SEImplementation/Plugin/PixelBoundaries/PixelBoundaries.h>
#include "SEImplementation/Plugin/DetectionFrameCoordinates/DetectionFrameCoordinates.h"

#include <SEImplementation/Plugin/MeasurementFrameRectangle/MeasurementFrameRectangle.h>
#include <SEImplementation/Plugin/MeasurementFrameRectangle/MeasurementFrameRectangleTask.h>

namespace SourceXtractor {

void MeasurementFrameRectangleTask::computeProperties(SourceInterface& source) const {
  auto& detection_group_stamp = source.getProperty<PixelBoundaries>();
  auto measurement_frame_coordinates = source.getProperty<MeasurementFrameCoordinates>(m_instance).getCoordinateSystem();
  auto detection_frame_coordinates = source.getProperty<DetectionFrameCoordinates>().getCoordinateSystem();

  const auto& measurement_frame_info = source.getProperty<MeasurementFrameInfo>(m_instance);

  // Get the coordinates of the detection frame group stamp
  auto stamp_top_left = detection_group_stamp.getMin();
  auto width = detection_group_stamp.getWidth();
  auto height = detection_group_stamp.getHeight();

  // Transform the 4 corner coordinates from detection image
  ImageCoordinate coord1, coord2, coord3, coord4;
  bool bad_coordinates = false;
  try {
    coord1 = measurement_frame_coordinates->worldToImage(
        detection_frame_coordinates->imageToWorld(ImageCoordinate(stamp_top_left.m_x, stamp_top_left.m_y)));
    coord2 = measurement_frame_coordinates->worldToImage(
        detection_frame_coordinates->imageToWorld(ImageCoordinate(stamp_top_left.m_x + width, stamp_top_left.m_y)));
    coord3 = measurement_frame_coordinates->worldToImage(
        detection_frame_coordinates->imageToWorld(ImageCoordinate(stamp_top_left.m_x + width, stamp_top_left.m_y + height)));
    coord4 = measurement_frame_coordinates->worldToImage(
        detection_frame_coordinates->imageToWorld(ImageCoordinate(stamp_top_left.m_x, stamp_top_left.m_y + height)));
  }
  catch (const InvalidCoordinatesException&) {
    bad_coordinates = true;
  }

  // Determine the min/max coordinates
  auto min_x = std::min(coord1.m_x, std::min(coord2.m_x, std::min(coord3.m_x, coord4.m_x)));
  auto min_y = std::min(coord1.m_y, std::min(coord2.m_y, std::min(coord3.m_y, coord4.m_y)));
  auto max_x = std::max(coord1.m_x, std::max(coord2.m_x, std::max(coord3.m_x, coord4.m_x)));
  auto max_y = std::max(coord1.m_y, std::max(coord2.m_y, std::max(coord3.m_y, coord4.m_y)));

  PixelCoordinate min_coord, max_coord;
  min_coord.m_x = int(min_x);
  min_coord.m_y = int(min_y);
  max_coord.m_x = int(max_x) + 1;
  max_coord.m_y = int(max_y) + 1;

  // The full boundaries may lie outside of the frame
  if (bad_coordinates || max_coord.m_x < 0 || max_coord.m_y < 0 ||
      min_coord.m_x >= measurement_frame_info.getWidth() || min_coord.m_y >= measurement_frame_info.getHeight()) {
    source.setIndexedProperty<MeasurementFrameRectangle>(m_instance, bad_coordinates);
  }
  // Clip the coordinates to fit the available image
  else {
    min_coord.m_x = std::max(0, min_coord.m_x);
    min_coord.m_y = std::max(0, min_coord.m_y);
    max_coord.m_x = std::min(measurement_frame_info.getWidth() - 1, max_coord.m_x);
    max_coord.m_y = std::min(measurement_frame_info.getHeight() - 1, max_coord.m_y);

    source.setIndexedProperty<MeasurementFrameRectangle>(m_instance, min_coord, max_coord);
  }
}

} // SEImplementation namespace

