/*  
 * Copyright (C) 2012-2020 Euclid Science Ground Segment    
 *  
 * This library is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General  
 * Public License as published by the Free Software Foundation; either version 3.0 of the License, or (at your option)  
 * any later version.  
 *  
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied  
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more  
 * details.  
 *  
 * You should have received a copy of the GNU Lesser General Public License along with this library; if not, write to  
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA  
 */    

/**
 * @file SourceFlagsSourceTask.h
 *
 * @date Jul 12, 2018
 * @author mkuemmel@usm.lmu.de
 */

#ifndef _SEIMPLEMENTATION_PLUGIN_SOURCEFLAGSOURCETASK_H_
#define _SEIMPLEMENTATION_PLUGIN_SOURCEFLAGSOURCETASK_H_

#include <iostream>

#include "SEImplementation/Plugin/SourceFlags/SourceFlags.h"
#include "SEFramework/Task/SourceTask.h"
#include "SEImplementation/Plugin/SaturateFlag/SaturateFlag.h"
#include "SEImplementation/Plugin/BoundaryFlag/BoundaryFlag.h"

namespace SExtractor {
class SourceFlagsSourceTask : public SourceTask {
public:
  virtual ~SourceFlagsSourceTask() = default;
  virtual void computeProperties(SourceInterface& source) const {
    long int source_flag(0);

    // add the saturate flag as "1"
    source_flag +=     source.getProperty<SaturateFlag>().getSaturateFlag();

    // add the saturate flag as "2"
    source_flag += 2 * source.getProperty<BoundaryFlag>().getBoundaryFlag();

    // set the combined source flag
    source.setProperty<SourceFlags>(source_flag);
  };
private:
}; // End of SourceFlagsSourceTask class
} // namespace SExtractor

#endif /* _SEIMPLEMENTATION_PLUGIN_SOURCEFLAGSOURCETASK_H_ */



