/*******************************************************************************
* Copyright 2023 Arm Ltd. and affiliates
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include "cpu/aarch64/acl_layer_normalization.hpp"

namespace dnnl {
namespace impl {
namespace cpu {
namespace aarch64 {

status_t acl_layer_normalization_fwd_t::execute(const exec_ctx_t &ctx) const {
    return execute_forward(ctx);
}

status_t acl_layer_normalization_fwd_t::init(engine_t *engine) {
    auto aep = pd()->anp;
    acl_obj.get()->configure(
            &aep.data_info, &aep.data_info, desc()->layer_norm_epsilon);
    return status::success;
}

status_t acl_layer_normalization_fwd_t::execute_forward(
        const exec_ctx_t &ctx) const {

    // Lock here is needed because resource_mapper does not support
    // concurrent access.
    std::lock_guard<std::mutex> _lock {this->mtx};

    // Retrieve primitive resource and configured Compute Library objects
    auto *acl_resource
            = ctx.get_resource_mapper()
                      ->get<acl_layer_normalization_resource_t>(this);
    acl_msdnorm_obj_t &acl_obj = acl_resource->get_acl_obj();

    auto src = CTX_IN_MEM(const float *, DNNL_ARG_SRC);

    arm_compute::Tensor data_tensor;

    auto const acp = pd()->anp;

    data_tensor.allocator()->init(acp.data_info);

    data_tensor.allocator()->import_memory(const_cast<float *>(src));

    arm_compute::Tensor data_tensor;

    auto const acp = pd()->anp;

    data_tensor.allocator()->init(acp.data_info);

    data_tensor.allocator()->import_memory(const_cast<float *>(src));

    auto dst = CTX_OUT_MEM(float *, DNNL_ARG_DST);
    acl_obj.dst_tensor.allocator()->import_memory(dst);

    acl_obj.msdNorm.run();

    acl_obj.src_tensor.allocator()->free();
    acl_obj.dst_tensor.allocator()->free();

    return status::success;
}

} // namespace aarch64
} // namespace cpu
} // namespace impl
} // namespace dnnl
