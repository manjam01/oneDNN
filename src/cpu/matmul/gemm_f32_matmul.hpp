/*******************************************************************************
* Copyright 2019-2025 Intel Corporation
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

#ifndef CPU_MATMUL_GEMM_F32_MATMUL_HPP
#define CPU_MATMUL_GEMM_F32_MATMUL_HPP

#include <assert.h>

#include "common/c_types_map.hpp"
#include "common/primitive.hpp"
#include "common/type_helpers.hpp"

#include "cpu/gemm_inner_product_utils.hpp"

#include "cpu/matmul/cpu_matmul_pd.hpp"
#include "cpu/matmul/gemm_based_common.hpp"

namespace dnnl {
namespace impl {
namespace cpu {
namespace matmul {

struct gemm_f32_matmul_t : public primitive_t {
    struct pd_t : public cpu_matmul_pd_t {
        using cpu_matmul_pd_t::cpu_matmul_pd_t;

        DECLARE_COMMON_PD_T("gemm:jit:f32", gemm_f32_matmul_t);

        status_t init(engine_t *engine);
        const gemm_based::params_t &params() const { return params_; }

        int nthr_; // To not exceed the limit in execute used for set up.

    private:
        status_t configure_attributes();
        gemm_based::params_t params_;
    };

    gemm_f32_matmul_t(const pd_t *apd) : primitive_t(apd) {}

    status_t init(engine_t *engine) override {
        if (pd()->params().has_pp_kernel_) {
            const bool has_runtime_dims
                    = memory_desc_wrapper(pd()->dst_md()).has_runtime_dims();
            const int nthr = pd()->nthr_;
            const dim_t batch = pd()->batch();
            const dim_t M = pd()->M();

            // mb value is calculated based on work-sharing using
            // balance211 in execute()
            dim_t mb = DNNL_RUNTIME_DIM_VAL;
            if (!has_runtime_dims && ((batch * M) % nthr == 0)) {
                const dim_t m_per_thr = nstl::max<dim_t>(1, (batch * M) / nthr);
                if (m_per_thr >= M && m_per_thr % M == 0) {
                    mb = M;
                } else if (m_per_thr < M && M % m_per_thr == 0) {
                    mb = m_per_thr;
                }
            }

            CHECK(safe_ptr_assign(pp_kernel_,
                    inner_product_utils::pp_kernel_t::create(pd()->N(), mb,
                            pd()->ldc(), &pd()->params().pp_attr_,
                            pd()->desc()->bias_desc.data_type,
                            pd()->desc()->accum_data_type, pd()->dst_md(),
                            pd()->params().skip_sum_)));
            return pp_kernel_->create_kernel();
        }

        return status::success;
    }

    static constexpr data_type_t src_type = data_type::f32;
    static constexpr data_type_t weights_type = data_type::f32;
    static constexpr data_type_t dst_type = data_type::f32;
    static constexpr data_type_t acc_type = data_type::f32;

    using src_data_t = typename prec_traits_t<src_type>::type;
    using weights_data_t = typename prec_traits_t<weights_type>::type;
    using dst_data_t = typename prec_traits_t<dst_type>::type;
    using acc_data_t = typename prec_traits_t<acc_type>::type;

    status_t execute(const exec_ctx_t &ctx) const override {
        return execute_ref(ctx);
    }

private:
    const pd_t *pd() const { return (const pd_t *)primitive_t::pd().get(); }
    status_t execute_ref(const exec_ctx_t &ctx) const;

    std::unique_ptr<inner_product_utils::pp_kernel_t> pp_kernel_;
};

} // namespace matmul
} // namespace cpu
} // namespace impl
} // namespace dnnl

#endif
