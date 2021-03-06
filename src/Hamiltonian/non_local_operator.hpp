// Copyright (c) 2013-2017 Anton Kozhevnikov, Thomas Schulthess
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that
// the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the
//    following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions
//    and the following disclaimer in the documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/** \file non_local_operator.hpp
 *
 *  \brief Contains declaration and implementation of sirius::Non_local_operator class.
 */

#ifndef __NON_LOCAL_OPERATOR_HPP__
#define __NON_LOCAL_OPERATOR_HPP__

#include "Beta_projectors/beta_projectors.hpp"

namespace sirius {

/// Non-local part of the Hamiltonian and S-operator in the pseudopotential method.
template <typename T>
class Non_local_operator
{
  protected:
    Simulation_context const& ctx_;

    device_t pu_;

    int packed_mtrx_size_;

    mdarray<int, 1> packed_mtrx_offset_;

    /// Non-local operator matrix.
    mdarray<T, 2> op_;

    mdarray<T, 1> work_;

    bool is_null_{false};

    /* copy assigment operrator is forbidden */
    Non_local_operator& operator=(Non_local_operator const& src) = delete;
    /* copy constructor is forbidden */
    Non_local_operator(Non_local_operator const& src) = delete;

  public:
    Non_local_operator(Simulation_context const& ctx__)
        : ctx_(ctx__)
    {
        PROFILE("sirius::Non_local_operator::Non_local_operator");
        pu_                 = this->ctx_.processing_unit();
        auto& uc            = this->ctx_.unit_cell();
        packed_mtrx_offset_ = mdarray<int, 1>(uc.num_atoms());
        packed_mtrx_size_   = 0;
        for (int ia = 0; ia < uc.num_atoms(); ia++) {
            int nbf                 = uc.atom(ia).mt_basis_size();
            packed_mtrx_offset_(ia) = packed_mtrx_size_;
            packed_mtrx_size_ += nbf * nbf;
        }

        switch (pu_) {
            case device_t::GPU: {
                packed_mtrx_offset_.allocate(memory_t::device).copy_to(memory_t::device);
                break;
            }
            case device_t::CPU: {
                break;
            }
        }
    }

    ~Non_local_operator()
    {
    }

    /// Apply chunk of beta-projectors to all wave functions.
    inline void apply(int chunk__, int ispn_block__, Wave_functions& op_phi__, int idx0__, int n__,
                      Beta_projectors_base& beta__, matrix<T>& beta_phi__);

    /// Apply beta projectors from one atom in a chunk of beta projectors to all wave-functions.
    inline void apply(int chunk__, int ia__, int ispn_block__, Wave_functions& op_phi__, int idx0__, int n__,
                      Beta_projectors_base& beta__, matrix<T>& beta_phi__);

    inline T operator()(int xi1__, int xi2__, int ia__)
    {
        return (*this)(xi1__, xi2__, 0, ia__);
    }

    inline T operator()(int xi1__, int xi2__, int ispn__, int ia__)
    {
        int nbf = this->ctx_.unit_cell().atom(ia__).mt_basis_size();
        return op_(packed_mtrx_offset_(ia__) + xi2__ * nbf + xi1__, ispn__);
    }
};

template <>
inline void Non_local_operator<double_complex>::apply(int chunk__, int ispn_block__, Wave_functions& op_phi__,
                                                      int idx0__, int n__, Beta_projectors_base& beta__,
                                                      matrix<double_complex>& beta_phi__)
{
    PROFILE("sirius::Non_local_operator::apply");

    if (is_null_) {
        return;
    }

    int jspn = ispn_block__ & 1;

    auto& beta_gk     = beta__.pw_coeffs_a();
    int num_gkvec_loc = beta__.num_gkvec_loc();
    int nbeta         = beta__.chunk(chunk__).num_beta_;

    if (static_cast<size_t>(nbeta * n__) > work_.size()) {
        work_ = mdarray<double_complex, 1>(nbeta * n__);
        if (pu_ == device_t::GPU) {
            work_.allocate(memory_t::device);
        }
    }
    /* setup linear algebra parameters */
    memory_t mem{memory_t::none};
    linalg_t la{linalg_t::none};
    switch (pu_) {
        case device_t::CPU: {
            mem = memory_t::host;
            la  = linalg_t::blas;
            break;
        }
        case device_t::GPU: {
            mem = memory_t::device;
            la  = linalg_t::gpublas;
            break;
        }
    }

    /* compute O * <beta|phi> for atoms in a chunk */
    #pragma omp parallel for
    for (int i = 0; i < beta__.chunk(chunk__).num_atoms_; i++) {
        /* number of beta functions for a given atom */
        int nbf  = beta__.chunk(chunk__).desc_(beta_desc_idx::nbf, i);
        int offs = beta__.chunk(chunk__).desc_(beta_desc_idx::offset, i);
        int ia   = beta__.chunk(chunk__).desc_(beta_desc_idx::ia, i);

        if (nbf) {
            linalg2(la).gemm('N', 'N', nbf, n__, nbf, &linalg_const<double_complex>::one(),
                             op_.at(mem, packed_mtrx_offset_(ia), ispn_block__), nbf, beta_phi__.at(mem, offs, 0), nbeta,
                             &linalg_const<double_complex>::zero(), work_.at(mem, offs), nbeta,
                             stream_id(omp_get_thread_num()));
        }
    }
    switch (pu_) {
        case device_t::GPU: {
            /* wait for previous zgemms */
            #pragma omp parallel
            acc::sync_stream(stream_id(omp_get_thread_num()));
            break;
        }
        case device_t::CPU: {
            break;
        }
    }

    /* compute <G+k|beta> * O * <beta|phi> and add to op_phi */
    linalg2(ctx_.blas_linalg_t()).gemm('N', 'N', num_gkvec_loc, n__, nbeta,
                                       &linalg_const<double_complex>::one(),
                                       beta_gk.at(mem), num_gkvec_loc, work_.at(mem), nbeta,
                                       &linalg_const<double_complex>::one(),
                                       op_phi__.pw_coeffs(jspn).prime().at(op_phi__.preferred_memory_t(), 0, idx0__),
                                       op_phi__.pw_coeffs(jspn).prime().ld());

    switch (pu_) {
        case device_t::GPU: {
            acc::sync_stream(stream_id(-1));
            break;
        }
        case device_t::CPU: {
            break;
        }
    }
}

template <>
inline void Non_local_operator<double_complex>::apply(int chunk__, int ia__, int ispn_block__, Wave_functions& op_phi__,
                                                      int idx0__, int n__, Beta_projectors_base& beta__,
                                                      matrix<double_complex>& beta_phi__)
{
    int jspn = ispn_block__ & 1;

    auto& beta_gk     = beta__.pw_coeffs_a();
    int num_gkvec_loc = beta__.num_gkvec_loc();
    int nbeta         = beta__.chunk(chunk__).num_beta_;

    if (static_cast<size_t>(nbeta * n__) > work_.size()) {
        work_ = mdarray<double_complex, 1>(nbeta * n__);
        if (pu_ == device_t::GPU) {
            work_.allocate(memory_t::device);
        }
    }

    int nbf  = beta__.chunk(chunk__).desc_(beta_desc_idx::nbf, ia__);
    int offs = beta__.chunk(chunk__).desc_(beta_desc_idx::offset, ia__);
    int ia   = beta__.chunk(chunk__).desc_(beta_desc_idx::ia, ia__);

    if (nbf == 0) {
        return;
    }

    work_.zero();
    
    /* setup linear algebra parameters */
    memory_t mem{memory_t::none};
    linalg_t la{linalg_t::none};
    
    switch (pu_) {
        case device_t::CPU: {
            mem = memory_t::host;
            la  = linalg_t::blas;
            break;
        }
        case device_t::GPU: {
            mem = memory_t::device;
            la  = linalg_t::gpublas;
            break;
        }
    }

    linalg2(la).gemm('N', 'N', nbf, n__, nbf,
                     &linalg_const<double_complex>::one(),
                     op_.at(mem, packed_mtrx_offset_(ia), ispn_block__), nbf,
                     beta_phi__.at(mem, offs, 0), nbeta,
                     &linalg_const<double_complex>::zero(),
                     work_.at(mem), nbf);

    linalg2(ctx_.blas_linalg_t()).gemm('N', 'N', num_gkvec_loc, n__, nbf,
                                       &linalg_const<double_complex>::one(),
                                       beta_gk.at(mem, 0, offs), num_gkvec_loc,
                                       work_.at(mem), nbf,
                                       &linalg_const<double_complex>::one(),
                                       op_phi__.pw_coeffs(jspn).prime().at(op_phi__.preferred_memory_t(), 0, idx0__),
                                       op_phi__.pw_coeffs(jspn).prime().ld());
    switch (pu_) {
        case device_t::CPU: {
            break;
        }
        case device_t::GPU: {
#ifdef __GPU
            acc::sync_stream(stream_id(-1));
#endif
            break;
        }
    }
}

template <>
inline void Non_local_operator<double>::apply(int chunk__, int ispn_block__, Wave_functions& op_phi__, int idx0__,
                                              int n__, Beta_projectors_base& beta__, matrix<double>& beta_phi__)
{
    PROFILE("sirius::Non_local_operator::apply");

    if (is_null_) {
        return;
    }

    int jspn = ispn_block__ & 1;

    auto& beta_gk     = beta__.pw_coeffs_a();
    int num_gkvec_loc = beta__.num_gkvec_loc();
    int nbeta         = beta__.chunk(chunk__).num_beta_;

    if (static_cast<size_t>(nbeta * n__) > work_.size()) {
        work_ = mdarray<double, 1>(nbeta * n__);
        if (pu_ == device_t::GPU) {
            work_.allocate(memory_t::device);
        }
    }

    /* setup linear algebra parameters */
    memory_t mem{memory_t::none};
    linalg_t la{linalg_t::none};
    switch (pu_) {
        case device_t::CPU: {
            mem = memory_t::host;
            la  = linalg_t::blas;
            break;
        }
        case device_t::GPU: {
            mem = memory_t::device;
            la  = linalg_t::gpublas;
            break;
        }
    }

    /* compute O * <beta|phi> for atoms in a chunk */
    #pragma omp parallel for
    for (int i = 0; i < beta__.chunk(chunk__).num_atoms_; i++) {
        /* number of beta functions for a given atom */
        int nbf  = beta__.chunk(chunk__).desc_(beta_desc_idx::nbf, i);
        int offs = beta__.chunk(chunk__).desc_(beta_desc_idx::offset, i);
        int ia   = beta__.chunk(chunk__).desc_(beta_desc_idx::ia, i);

        if (nbf == 0) {
            continue;
        }
        linalg2(la).gemm('N', 'N', nbf, n__, nbf,
                         &linalg_const<double>::one(),
                         op_.at(mem, packed_mtrx_offset_(ia), ispn_block__), nbf,
                         beta_phi__.at(mem, offs, 0), nbeta,
                         &linalg_const<double>::zero(),
                         work_.at(mem, offs), nbeta,
                         stream_id(omp_get_thread_num()));
    }
    switch (pu_) {
        case device_t::GPU: {
            /* wait for previous zgemms */
            #pragma omp parallel
            acc::sync_stream(stream_id(omp_get_thread_num()));
            break;
        }
        case device_t::CPU: {
            break;
        }
    }

    /* compute <G+k|beta> * O * <beta|phi> and add to op_phi */
    linalg2(ctx_.blas_linalg_t()).gemm('N', 'N', 2 * num_gkvec_loc, n__, nbeta,
                                       &linalg_const<double>::one(),
                                       reinterpret_cast<double*>(beta_gk.at(mem)), 2 * num_gkvec_loc,
                                       work_.at(mem), nbeta,
                                       &linalg_const<double>::one(),
                                       reinterpret_cast<double*>(op_phi__.pw_coeffs(jspn).prime().at(op_phi__.preferred_memory_t(), 0, idx0__)),
                                       2 * op_phi__.pw_coeffs(jspn).prime().ld());

    switch (pu_) {
        case device_t::GPU: {
            acc::sync_stream(stream_id(-1));
            break;
        }
        case device_t::CPU: {
            break;
        }
    }
}

template <>
inline void Non_local_operator<double>::apply(int chunk__, int ia__, int ispn_block__, Wave_functions& op_phi__,
                                              int idx0__, int n__, Beta_projectors_base& beta__,
                                              matrix<double>& beta_phi__)
{
    TERMINATE_NOT_IMPLEMENTED;
}

template <typename T>
class D_operator : public Non_local_operator<T>
{
  private:
    void initialize()
    {
        auto& uc = this->ctx_.unit_cell();

        for (int ia = 0; ia < uc.num_atoms(); ia++) {
            int nbf = uc.atom(ia).mt_basis_size();
            if (uc.atom(ia).type().spin_orbit_coupling()) {

                // the pseudo potential contains information about
                // spin orbit coupling so we use a different formula
                // Eq.19 PRB 71 115106 for calculating the D matrix

                // Note that the D matrices are stored and
                // calculated in the up-down basis already not the
                // (Veff,Bx,By,Bz) one.
                for (int xi2 = 0; xi2 < nbf; xi2++) {
                    for (int xi1 = 0; xi1 < nbf; xi1++) {
                        int idx = xi2 * nbf + xi1;
                        for (int s = 0; s < 4; s++) {
                            this->op_(this->packed_mtrx_offset_(ia) + idx, s) =
                                utils::zero_if_not_complex<T>(uc.atom(ia).d_mtrx_so(xi1, xi2, s));
                        }
                    }
                }
            } else {
                // No spin orbit coupling for this atom \f[D = D(V_{eff})
                // I + D(B_x) \sigma_x + D(B_y) sigma_y + D(B_z)
                // sigma_z\f] since the D matrices are calculated that
                // way.
                for (int xi2 = 0; xi2 < nbf; xi2++) {
                    for (int xi1 = 0; xi1 < nbf; xi1++) {
                        int idx = xi2 * nbf + xi1;
                        switch (this->ctx_.num_mag_dims()) {
                            case 3: {
                                double bx = uc.atom(ia).d_mtrx(xi1, xi2, 2);
                                double by = uc.atom(ia).d_mtrx(xi1, xi2, 3);
                                this->op_(this->packed_mtrx_offset_(ia) + idx, 2) =
                                    utils::zero_if_not_complex<T>(double_complex(bx, -by));
                                this->op_(this->packed_mtrx_offset_(ia) + idx, 3) =
                                    utils::zero_if_not_complex<T>(double_complex(bx, by));
                            }
                            case 1: {
                                double v  = uc.atom(ia).d_mtrx(xi1, xi2, 0);
                                double bz = uc.atom(ia).d_mtrx(xi1, xi2, 1);

                                this->op_(this->packed_mtrx_offset_(ia) + idx, 0) = v + bz;
                                this->op_(this->packed_mtrx_offset_(ia) + idx, 1) = v - bz;
                                break;
                            }
                            case 0: {
                                this->op_(this->packed_mtrx_offset_(ia) + idx, 0) = uc.atom(ia).d_mtrx(xi1, xi2, 0);
                                break;
                            }
                            default: {
                                TERMINATE("wrong number of magnetic dimensions");
                            }
                        }
                    }
                }
            }
        }

        if (this->ctx_.control().print_checksum_ && this->ctx_.comm().rank() == 0) {
            auto cs = this->op_.checksum();
            utils::print_checksum("D_operator", cs);
        }

        if (this->pu_ == device_t::GPU) {
            this->op_.allocate(memory_t::device).copy_to(memory_t::device);
        }
    }

  public:
    D_operator(Simulation_context const& ctx_)
        : Non_local_operator<T>(ctx_)
    {
        this->op_ = mdarray<T, 2>(this->packed_mtrx_size_, ctx_.num_mag_dims() + 1);
        this->op_.zero();
        /* D-matrix is complex in non-collinear case */
        if (ctx_.num_mag_dims() == 3) {
            assert((std::is_same<T, double_complex>::value));
        }
        initialize();
    }
};

template <typename T>
class Q_operator : public Non_local_operator<T>
{
  private:
    void initialize()
    {
        auto& uc = this->ctx_.unit_cell();
        for (int ia = 0; ia < uc.num_atoms(); ia++) {
            int iat = uc.atom(ia).type().id();
            if (!uc.atom_type(iat).augment()) {
                continue;
            }
            int nbf = uc.atom(ia).mt_basis_size();
            for (int xi2 = 0; xi2 < nbf; xi2++) {
                for (int xi1 = 0; xi1 < nbf; xi1++) {
                    /* The ultra soft pseudo potential has spin orbit coupling incorporated to it, so we
                       need to rotate the Q matrix */
                    if (this->ctx_.unit_cell().atom_type(iat).spin_orbit_coupling()) {
                        /* this is nothing else than Eq.18 of Ref PRB 71, 115106 */
                        for (auto si = 0; si < 2; si++) {
                            for (auto sj = 0; sj < 2; sj++) {

                                double_complex result(0, 0);

                                for (int xi2p = 0; xi2p < nbf; xi2p++) {
                                    if (uc.atom(ia).type().compare_index_beta_functions(xi2, xi2p)) {
                                        for (int xi1p = 0; xi1p < nbf; xi1p++) {
                                            /* The F coefficients are already "block diagonal" so we do a full
                                               summation. We actually rotate the q_matrices only */
                                            if (uc.atom(ia).type().compare_index_beta_functions(xi1, xi1p)) {
                                                result += this->ctx_.augmentation_op(iat).q_mtrx(xi1p, xi2p) *
                                                          (uc.atom(ia).type().f_coefficients(xi1, xi1p, sj, 0) *
                                                               uc.atom(ia).type().f_coefficients(xi2p, xi2, 0, si) +
                                                           uc.atom(ia).type().f_coefficients(xi1, xi1p, sj, 1) *
                                                               uc.atom(ia).type().f_coefficients(xi2p, xi2, 1, si));
                                            }
                                        }
                                    }
                                }

                                /* the order of the index is important */
                                const int ind = (si == sj) ? si : sj + 2;
                                /* this gives
                                   ind = 0 if si = up and sj = up
                                   ind = 1 if si = sj = down
                                   ind = 2 if si = down and sj = up
                                   ind = 3 if si = up and sj = down */
                                this->op_(this->packed_mtrx_offset_(ia) + xi2 * nbf + xi1, ind) =
                                    // type_wrapper<T>::bypass(result);
                                    utils::zero_if_not_complex<T>(result);
                            }
                        }
                    } else {
                        for (int ispn = 0; ispn < this->ctx_.num_spins(); ispn++) {
                            this->op_(this->packed_mtrx_offset_(ia) + xi2 * nbf + xi1, ispn) =
                                this->ctx_.augmentation_op(iat).q_mtrx(xi1, xi2);
                        }
                    }
                }
            }
        }
        if (this->ctx_.control().print_checksum_ && this->ctx_.comm().rank() == 0) {
            auto cs = this->op_.checksum();
            utils::print_checksum("Q_operator", cs);
        }

        if (this->pu_ == device_t::GPU) {
            this->op_.allocate(memory_t::device).copy_to(memory_t::device);
        }
    }

  public:
    Q_operator(Simulation_context const& ctx_)
        : Non_local_operator<T>(ctx_)
    {
        /* Q-operator is independent of spin if there is no spin-orbit; however, it simplifies the apply()
         * method if the Q-operator has a spin index */
        this->op_ = mdarray<T, 2>(this->packed_mtrx_size_, ctx_.num_mag_dims() + 1);
        this->op_.zero();
        initialize();
    }
};

template <typename T>
class P_operator : public Non_local_operator<T>
{
  public:
    P_operator(Simulation_context const& ctx_, mdarray<double_complex, 3>& p_mtrx__)
        : Non_local_operator<T>(ctx_)
    {
        /* Q-operator is independent of spin */
        this->op_ = mdarray<T, 2>(this->packed_mtrx_size_, 1);
        this->op_.zero();

        auto& uc = ctx_.unit_cell();
        for (int ia = 0; ia < uc.num_atoms(); ia++) {
            int iat = uc.atom(ia).type().id();
            if (!uc.atom_type(iat).augment()) {
                continue;
            }
            int nbf = uc.atom(ia).mt_basis_size();
            for (int xi2 = 0; xi2 < nbf; xi2++) {
                for (int xi1 = 0; xi1 < nbf; xi1++) {
                    this->op_(this->packed_mtrx_offset_(ia) + xi2 * nbf + xi1, 0) = -p_mtrx__(xi1, xi2, iat).real();
                }
            }
        }
        if (this->pu_ == device_t::GPU) {
            this->op_.allocate(memory_t::device);
            this->op_.copy_to(memory_t::device);
        }
    }
};

} // namespace sirius

#endif
