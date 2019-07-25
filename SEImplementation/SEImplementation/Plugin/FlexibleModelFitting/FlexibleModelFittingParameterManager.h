/*
 * FlexibleModelFittingParameterManager.h
 *
 *  Created on: Oct 9, 2018
 *      Author: mschefer
 */

#ifndef _SEIMPLEMENTATION_PLUGIN_FLEXIBLEMODELFITTING_FLEXIBLEMODELFITTINGPARAMETERMANAGER_H_
#define _SEIMPLEMENTATION_PLUGIN_FLEXIBLEMODELFITTING_FLEXIBLEMODELFITTINGPARAMETERMANAGER_H_

#include <map>
#include <vector>

#include "ModelFitting/Engine/EngineParameterManager.h"
#include "SEFramework/Source/SourceInterface.h"
#include "SEImplementation/Plugin/FlexibleModelFitting/FlexibleModelFittingParameter.h"

namespace SExtractor {

// Needed to store the source in a reference_wrapper
inline bool operator<(std::reference_wrapper<const SourceInterface> a, std::reference_wrapper<const SourceInterface> b) {
  return &a.get() < &b.get();
}

class FlexibleModelFittingParameter;

class FlexibleModelFittingParameterManager {

public:

  FlexibleModelFittingParameterManager() {}
  virtual ~FlexibleModelFittingParameterManager() {}


  std::shared_ptr<ModelFitting::BasicParameter> getParameter(
      const SourceInterface& source, std::shared_ptr<FlexibleModelFittingParameter> parameter) const {
    auto key = std::make_tuple(std::cref(source), parameter);
    m_accessed_params.insert(key);
    followDependencies(source, parameter);
    return m_params.at(key);
  }

  void addParameter(const SourceInterface& source, std::shared_ptr<FlexibleModelFittingParameter> parameter,
      std::shared_ptr<ModelFitting::BasicParameter> engine_parameter) {
    m_params[std::make_tuple(std::cref(source), parameter)] = engine_parameter;
  }

  // Used to just store a parameter's pointer for the duration of the model fitting
  // This is necessary to keep dependent parameters working
  void storeParameter(std::shared_ptr<ModelFitting::BasicParameter> param) {
    m_storage.emplace_back(param);
  }

  int getParameterNb() const {
    return m_params.size();
  }

  void clearAccessCheck() {
    m_accessed_params.clear();
  }

  bool isParamAccessed(const SourceInterface& source, std::shared_ptr<FlexibleModelFittingParameter> parameter) const {
    auto key = std::make_tuple(std::cref(source), parameter);
    return m_accessed_params.count(key) > 0;
  }

private:
  std::map<std::tuple<std::reference_wrapper<const SourceInterface>, std::shared_ptr<FlexibleModelFittingParameter>>, std::shared_ptr<ModelFitting::BasicParameter>> m_params;
  mutable std::set<std::tuple<std::reference_wrapper<const SourceInterface>, std::shared_ptr<FlexibleModelFittingParameter>>> m_accessed_params;
  std::vector<std::shared_ptr<ModelFitting::BasicParameter>> m_storage;

  // Propagate access to dependees
  void followDependencies(const SourceInterface& source, std::shared_ptr<FlexibleModelFittingParameter> parameter) const {
    auto dependent_parameter = std::dynamic_pointer_cast<FlexibleModelFittingDependentParameter>(parameter).get();
    if (dependent_parameter) {
      for (auto &dependee : dependent_parameter->getDependees()) {
        auto key_dependee = std::make_tuple(std::cref(source), dependee);
        m_accessed_params.insert(key_dependee);
        followDependencies(source, dependee);
      }
    }
  }
};

}



#endif /* _SEIMPLEMENTATION_PLUGIN_FLEXIBLEMODELFITTING_FLEXIBLEMODELFITTINGPARAMETERMANAGER_H_ */
