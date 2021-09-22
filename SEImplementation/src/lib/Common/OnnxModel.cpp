/*
 * OnnxModel.cpp
 *
 *  Created on: Feb 16, 2021
 *      Author: mschefer
 */

#include "ElementsKernel/Exception.h"
#include "ElementsKernel/Logging.h"
#include "AlexandriaKernel/memory_tools.h"

#include "SEImplementation/Common/OnnxModel.h"
#include "SEImplementation/Common/OnnxCommon.h"

namespace SourceXtractor {

namespace {
/**
 * Pretty-print a vector with shape information
 */
static std::string formatShape(const std::vector<int64_t>& shape) {
  std::ostringstream stream;
  for (auto i = shape.begin(); i != shape.end() - 1; ++i) {
    stream << *i << " x ";
  }
  stream << shape.back();
  return stream.str();
}

}

OnnxModel::OnnxModel(const std::string& model_path) {
  m_model_path = model_path;

  Elements::Logging onnx_logger = Elements::Logging::getLogger("Onnx");
  auto allocator = Ort::AllocatorWithDefaultOptions();

  onnx_logger.info() << "Loading ONNX model " << model_path;
  m_session = Euclid::make_unique<Ort::Session>(ORT_ENV, model_path.c_str(), Ort::SessionOptions{nullptr});

//  if (m_session->GetInputCount() != 1) {
//    throw Elements::Exception() << "Only ONNX models with a single input tensor are supported";
//  }
  if (m_session->GetOutputCount() != 1) {
    throw Elements::Exception() << "Only ONNX models with a single output tensor are supported";
  }

  for (int i=0; i<m_session->GetInputCount(); i++) {
    auto input_type = m_session->GetInputTypeInfo(i);

    m_input_names.emplace_back(m_session->GetInputName(i, allocator));
    m_input_shapes.emplace_back(input_type.GetTensorTypeAndShapeInfo().GetShape());
    m_input_types.emplace_back(input_type.GetTensorTypeAndShapeInfo().GetElementType());
  }

  m_output_name = m_session->GetOutputName(0, allocator);
  m_domain_name = m_session->GetModelMetadata().GetDomain(allocator);
  m_graph_name = m_session->GetModelMetadata().GetGraphName(allocator);

  auto output_type = m_session->GetOutputTypeInfo(0);

  m_output_shape = output_type.GetTensorTypeAndShapeInfo().GetShape();
  m_output_type = output_type.GetTensorTypeAndShapeInfo().GetElementType();

//  onnx_logger.info() << "ONNX model with input of " << formatShape(m_input_shapes[0]);
//  onnx_logger.info() << "ONNX model with output of " << formatShape(m_output_shape);
}

}
