/**
 * @file SEMain/SExtractorConfig.h
 * @date 07/20/16
 * @author nikoapos
 */

#ifndef _SEMAIN_SEXTRACTORCONFIG_H
#define _SEMAIN_SEXTRACTORCONFIG_H

#include "Configuration/Configuration.h"

namespace SExtractor {

/**
 * @class SExtractorConfig
 * @brief
 *
 */
class SExtractorConfig : public Euclid::Configuration::Configuration {

public:

  /**
   * @brief Destructor
   */
  virtual ~SExtractorConfig() = default;

  SExtractorConfig(long manager_id);
  
  std::map<std::string, OptionDescriptionList> getProgramOptions() override;

  void initialize(const UserValues& args) override;
  
  bool listOutputColumns() const;

private:
  
  bool m_list_columns_flag;

}; /* End of SExtractorConfig class */

} /* namespace SExtractor */


#endif
