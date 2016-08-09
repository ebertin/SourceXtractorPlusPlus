/**
 * @file src/program/SExtractor.cpp
 * @date 05/31/16
 * @author mschefer
 */

#include <map>
#include <string>
#include <boost/program_options.hpp>

#include <iomanip>

#include "ElementsKernel/ProgramHeaders.h"

#include "SEFramework/Plugin/PluginManager.h"

#include "SEFramework/Task/TaskProvider.h"
#include "SEFramework/Image/SubtractImage.h"
#include "SEFramework/Pipeline/SourceGrouping.h"
#include "SEFramework/Pipeline/Deblending.h"
#include "SEFramework/Pipeline/Partition.h"
#include "SEFramework/Output/OutputRegistry.h"

#include "SEFramework/Task/TaskFactoryRegistry.h"

#include "SEFramework/Source/SourceWithOnDemandPropertiesFactory.h"
#include "SEFramework/Source/SourceGroupWithOnDemandPropertiesFactory.h"

#include "SEImplementation/Segmentation/SegmentationFactory.h"
#include "SEImplementation/Output/OutputFactory.h"

#include "SEImplementation/Property/PixelCentroid.h"
#include "SEImplementation/Property/ExternalFlag.h"

#include "SEImplementation/Partition/PartitionFactory.h"
#include "SEImplementation/Grouping/OverlappingBoundariesCriteria.h"

#include "SEImplementation/Configuration/DetectionImageConfig.h"
#include "SEImplementation/Configuration/SegmentationConfig.h"
#include "SEImplementation/Configuration/ExternalFlagConfig.h"

#include "SEMain/SExtractorConfig.h"

#include "Configuration/ConfigManager.h"
#include "Configuration/Utils.h"

namespace po = boost::program_options;
using namespace SExtractor;
using namespace Euclid::Configuration;

static long config_manager_id = getUniqueManagerId();

class GroupObserver : public Observer<std::shared_ptr<SourceGroupInterface>> {
public:
  virtual void handleMessage(const std::shared_ptr<SourceGroupInterface>& group) override {
      m_list.push_back(group);
  }

  std::list<std::shared_ptr<SourceGroupInterface>> m_list;
};

class SourceObserver : public Observer<std::shared_ptr<SourceWithOnDemandProperties>> {
public:
  virtual void handleMessage(const std::shared_ptr<SourceWithOnDemandProperties>& source) override {
      m_list.push_back(source);
  }

  std::list<std::shared_ptr<SourceWithOnDemandProperties>> m_list;
};

static Elements::Logging logger = Elements::Logging::getLogger("SExtractor");

class SEMain : public Elements::Program {
  
  std::shared_ptr<TaskFactoryRegistry> task_factory_registry = std::make_shared<TaskFactoryRegistry>();
  std::shared_ptr<TaskProvider> task_provider = std::make_shared<TaskProvider>(task_factory_registry);
  std::shared_ptr<OutputRegistry> output_registry = std::make_shared<OutputRegistry>();
  SegmentationFactory segmentation_factory {task_provider};
  OutputFactory output_factory { output_registry };
  PluginManager plugin_manager { task_factory_registry, output_registry };
  std::shared_ptr<SourceFactory> source_factory =
      std::make_shared<SourceWithOnDemandPropertiesFactory>(task_provider);
  std::shared_ptr<SourceGroupFactory> group_factory =
          std::make_shared<SourceGroupWithOnDemandPropertiesFactory>(task_provider);
  PartitionFactory partition_factory {source_factory};

public:
  
  SEMain() {
  }

  po::options_description defineSpecificProgramOptions() override {
    auto& config_manager = ConfigManager::getInstance(config_manager_id);
    config_manager.registerConfiguration<SExtractorConfig>();

    plugin_manager.loadPlugins(); // FIXME currently loading only static plugins here
    plugin_manager.reportConfigDependencies(config_manager);

    task_factory_registry->reportConfigDependencies(config_manager);
    segmentation_factory.reportConfigDependencies(config_manager);
    partition_factory.reportConfigDependencies(config_manager);
    output_factory.reportConfigDependencies(config_manager);
    return config_manager.closeRegistration();
  }


  Elements::ExitCode mainMethod(std::map<std::string, po::variable_value>& args) override {

    auto& config_manager = ConfigManager::getInstance(config_manager_id);

    config_manager.initialize(args);

    plugin_manager.configure(config_manager);

    task_factory_registry->configure(config_manager);
    task_factory_registry->registerPropertyInstances(*output_registry);
    
    segmentation_factory.configure(config_manager);
    partition_factory.configure(config_manager);
    output_factory.configure(config_manager);
    
    auto source_observer = std::make_shared<SourceObserver>();
    auto group_observer = std::make_shared<GroupObserver>();

    auto detection_image = config_manager.getConfiguration<DetectionImageConfig>().getDetectionImage();

    auto segmentation = segmentation_factory.getSegmentation();

    auto partition = partition_factory.getPartition();
    
    auto source_grouping = std::make_shared<SourceGrouping>(
        std::unique_ptr<OverlappingBoundariesCriteria>(new OverlappingBoundariesCriteria), group_factory);
    auto deblending = std::make_shared<Deblending>(std::vector<std::shared_ptr<DeblendAction>>());

    std::shared_ptr<Output> output = output_factory.getOutput();

    // Link together the pipeline's steps
    segmentation->addObserver(partition);
    partition->addObserver(source_grouping);
    source_grouping->addObserver(deblending);
    deblending->addObserver(output);

    // Process the image
    segmentation->scan(*detection_image);

    SelectAllCriteria select_all_criteria;
    source_grouping->handleMessage(ProcessSourcesEvent(select_all_criteria));

    return Elements::ExitCode::OK;
  }

};

MAIN_FOR(SEMain)



