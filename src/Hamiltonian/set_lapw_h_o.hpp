// Copyright (c) 2013-2016 Anton Kozhevnikov, Thomas Schulthess
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

/** \file set_lapw_h_o.hpp
 *
 *  \brief Contains functions of LAPW Hamiltonian and overlap setup.
 */

template<>
inline void Hamiltonian::set_fv_h_o<CPU, electronic_structure_method_t::full_potential_lapwlo>(K_point* kp__,
                                                                                               dmatrix<double_complex>& h__,
                                                                                               dmatrix<double_complex>& o__) const
{
    PROFILE("sirius::Hamiltonian::set_fv_h_o");

    h__.zero();
    o__.zero();

    int num_atoms_in_block = 2 * omp_get_max_threads();
    int nblk = unit_cell_.num_atoms() / num_atoms_in_block +
               std::min(1, unit_cell_.num_atoms() % num_atoms_in_block);

    int max_mt_aw = num_atoms_in_block * unit_cell_.max_mt_aw_basis_size();

    mdarray<double_complex, 2> alm_row(kp__->num_gkvec_row(), max_mt_aw);
    mdarray<double_complex, 2> alm_col(kp__->num_gkvec_col(), max_mt_aw);
    mdarray<double_complex, 2> halm_col(kp__->num_gkvec_col(), max_mt_aw);
    mdarray<double_complex, 2> oalm_col;
    if (ctx_.valence_relativity() == relativity_t::iora) {
        oalm_col = mdarray<double_complex, 2>(kp__->num_gkvec_col(), max_mt_aw);
    } else {
        oalm_col = mdarray<double_complex, 2>(alm_col.at(memory_t::host), kp__->num_gkvec_col(), max_mt_aw);
    }

    utils::timer t1("sirius::Hamiltonian::set_fv_h_o|zgemm");
    /* loop over blocks of atoms */
    for (int iblk = 0; iblk < nblk; iblk++) {
        /* number of matching AW coefficients in the block */
        int num_mt_aw{0};
        /* offsets for matching coefficients of individual atoms in the AW block */
        std::vector<int> offsets(num_atoms_in_block);
        for (int ia = iblk * num_atoms_in_block; ia < std::min(unit_cell_.num_atoms(), (iblk + 1) * num_atoms_in_block); ia++) {
            offsets[ia - iblk * num_atoms_in_block] = num_mt_aw;
            num_mt_aw += unit_cell_.atom(ia).type().mt_aw_basis_size();
        }

        if (ctx_.control().print_checksum_) {
            alm_row.zero();
            alm_col.zero();
            halm_col.zero();
        }

        #pragma omp parallel
        {
            int tid = omp_get_thread_num();
            for (int ia = iblk * num_atoms_in_block; ia < std::min(unit_cell_.num_atoms(), (iblk + 1) * num_atoms_in_block); ia++) {
                if (ia % omp_get_num_threads() == tid) {
                    int ialoc = ia - iblk * num_atoms_in_block;
                    auto& atom = unit_cell_.atom(ia);
                    auto& type = atom.type();
                    int naw = type.mt_aw_basis_size();

                    mdarray<double_complex, 2> alm_row_tmp(alm_row.at(memory_t::host, 0, offsets[ialoc]), kp__->num_gkvec_row(), naw);
                    mdarray<double_complex, 2> alm_col_tmp(alm_col.at(memory_t::host, 0, offsets[ialoc]), kp__->num_gkvec_col(), naw);
                    mdarray<double_complex, 2> halm_col_tmp(halm_col.at(memory_t::host, 0, offsets[ialoc]), kp__->num_gkvec_col(), naw);
                    mdarray<double_complex, 2> oalm_col_tmp;
                    if (ctx_.valence_relativity() == relativity_t::iora) {
                        oalm_col_tmp = mdarray<double_complex, 2>(oalm_col.at(memory_t::host, 0, offsets[ialoc]), kp__->num_gkvec_col(), naw);
                    } else {
                        oalm_col_tmp = mdarray<double_complex, 2>(alm_col.at(memory_t::host, 0, offsets[ialoc]), kp__->num_gkvec_col(), naw);
                    }

                    kp__->alm_coeffs_row().generate(ia, alm_row_tmp);
                    for (int xi = 0; xi < naw; xi++) {
                        for (int igk = 0; igk < kp__->num_gkvec_row(); igk++) {
                            alm_row_tmp(igk, xi) = std::conj(alm_row_tmp(igk, xi));
                        }
                    }
                    kp__->alm_coeffs_col().generate(ia, alm_col_tmp);
                    apply_hmt_to_apw<spin_block_t::nm>(atom, kp__->num_gkvec_col(), alm_col_tmp, halm_col_tmp);

                    if (ctx_.valence_relativity() == relativity_t::iora) {
                        alm_col_tmp >> oalm_col_tmp;
                        apply_o1mt_to_apw(atom, kp__->num_gkvec_col(), alm_col_tmp, oalm_col_tmp);
                    }

                    /* setup apw-lo and lo-apw blocks */
                    set_fv_h_o_apw_lo(kp__, type, atom, ia, alm_row_tmp, alm_col_tmp, h__, o__);
                }
            }
        }
        if (ctx_.control().print_checksum_) {
            double_complex z1 = alm_row.checksum();
            double_complex z2 = alm_col.checksum();
            double_complex z3 = halm_col.checksum();
            utils::print_checksum("alm_row", z1);
            utils::print_checksum("alm_col", z2);
            utils::print_checksum("halm_col", z3);
        }
        linalg<CPU>::gemm(0, 1, kp__->num_gkvec_row(), kp__->num_gkvec_col(), num_mt_aw,
                          linalg_const<double_complex>::one(),
                          alm_row.at(memory_t::host), alm_row.ld(),
                          oalm_col.at(memory_t::host), oalm_col.ld(),
                          linalg_const<double_complex>::one(),
                          o__.at(memory_t::host), o__.ld());

        linalg<CPU>::gemm(0, 1, kp__->num_gkvec_row(), kp__->num_gkvec_col(), num_mt_aw,
                          linalg_const<double_complex>::one(),
                          alm_row.at(memory_t::host), alm_row.ld(),
                          halm_col.at(memory_t::host), halm_col.ld(),
                          linalg_const<double_complex>::one(),
                          h__.at(memory_t::host), h__.ld());
    }
    double tval = t1.stop();
    if (kp__->comm().rank() == 0 && ctx_.control().print_performance_) {
        printf("effective zgemm performance: %12.6f GFlops",
               2 * 8e-9 * kp__->num_gkvec() * kp__->num_gkvec() * unit_cell_.mt_aw_basis_size() / tval);
    }

    /* add interstitial contributon */
    this->set_fv_h_o_it(kp__, h__, o__);

    /* setup lo-lo block */
    this->set_fv_h_o_lo_lo(kp__, h__, o__);
}

#ifdef __GPU
template<>
inline void Hamiltonian::set_fv_h_o<GPU, electronic_structure_method_t::full_potential_lapwlo>(K_point* kp__,
                                                                                               dmatrix<double_complex>& h__,
                                                                                               dmatrix<double_complex>& o__) const
{
    PROFILE("sirius::Hamiltonian::set_fv_h_o");

    utils::timer t2("sirius::Hamiltonian::set_fv_h_o|alloc");
    h__.allocate(memory_t::device);
    h__.zero(memory_t::host);
    h__.zero(memory_t::device);

    o__.allocate(memory_t::device);
    o__.zero(memory_t::host);
    o__.zero(memory_t::device);

    int num_atoms_in_block = 2 * omp_get_max_threads();
    int nblk = unit_cell_.num_atoms() / num_atoms_in_block +
               std::min(1, unit_cell_.num_atoms() % num_atoms_in_block);

    int max_mt_aw = num_atoms_in_block * unit_cell_.max_mt_aw_basis_size();

    mdarray<double_complex, 3> alm_row(kp__->num_gkvec_row(), max_mt_aw, 2, memory_t::host_pinned);
    alm_row.allocate(memory_t::device);

    mdarray<double_complex, 3> alm_col(kp__->num_gkvec_col(), max_mt_aw, 2, memory_t::host_pinned);
    alm_col.allocate(memory_t::device);

    mdarray<double_complex, 3> halm_col(kp__->num_gkvec_col(), max_mt_aw, 2, memory_t::host_pinned);
    halm_col.allocate(memory_t::device);
    t2.stop();

    if (ctx_.comm().rank() == 0 && ctx_.control().print_memory_usage_) {
        MEMORY_USAGE_INFO();
    }

    utils::timer t1("sirius::Hamiltonian::set_fv_h_o|zgemm");
    for (int iblk = 0; iblk < nblk; iblk++) {
        int num_mt_aw = 0;
        std::vector<int> offsets(num_atoms_in_block);
        for (int ia = iblk * num_atoms_in_block; ia < std::min(unit_cell_.num_atoms(), (iblk + 1) * num_atoms_in_block); ia++) {
            int ialoc = ia - iblk * num_atoms_in_block;
            auto& atom = unit_cell_.atom(ia);
            auto& type = atom.type();
            offsets[ialoc] = num_mt_aw;
            num_mt_aw += type.mt_aw_basis_size();
        }

        int s = iblk % 2;

        #pragma omp parallel
        {
            int tid = omp_get_thread_num();
            for (int ia = iblk * num_atoms_in_block; ia < std::min(unit_cell_.num_atoms(), (iblk + 1) * num_atoms_in_block); ia++) {
                if (ia % omp_get_num_threads() == tid) {
                    int ialoc = ia - iblk * num_atoms_in_block;
                    auto& atom = unit_cell_.atom(ia);
                    auto& type = atom.type();

                    mdarray<double_complex, 2> alm_row_tmp(alm_row.at(memory_t::host, 0, offsets[ialoc], s),
                                                           alm_row.at(memory_t::device, 0, offsets[ialoc], s),
                                                           kp__->num_gkvec_row(), type.mt_aw_basis_size());

                    mdarray<double_complex, 2> alm_col_tmp(alm_col.at(memory_t::host, 0, offsets[ialoc], s),
                                                           alm_col.at(memory_t::device, 0, offsets[ialoc], s),
                                                           kp__->num_gkvec_col(), type.mt_aw_basis_size());

                    mdarray<double_complex, 2> halm_col_tmp(halm_col.at(memory_t::host, 0, offsets[ialoc], s),
                                                            halm_col.at(memory_t::device, 0, offsets[ialoc], s),
                                                            kp__->num_gkvec_col(), type.mt_aw_basis_size());

                    kp__->alm_coeffs_row().generate(ia, alm_row_tmp);
                    for (int xi = 0; xi < type.mt_aw_basis_size(); xi++) {
                        for (int igk = 0; igk < kp__->num_gkvec_row(); igk++) {
                            alm_row_tmp(igk, xi) = std::conj(alm_row_tmp(igk, xi));
                        }
                    }
                    alm_row_tmp.copy_to(memory_t::device, stream_id(tid));

                    kp__->alm_coeffs_col().generate(ia, alm_col_tmp);
                    alm_col_tmp.copy_to(memory_t::device, stream_id(tid));

                    apply_hmt_to_apw<spin_block_t::nm>(atom, kp__->num_gkvec_col(), alm_col_tmp, halm_col_tmp);
                    halm_col_tmp.copy_to(memory_t::device, stream_id(tid));

                    /* setup apw-lo and lo-apw blocks */
                    set_fv_h_o_apw_lo(kp__, type, atom, ia, alm_row_tmp, alm_col_tmp, h__, o__);
                }
            }
            acc::sync_stream(stream_id(tid));
        }
        acc::sync_stream(stream_id(omp_get_max_threads()));
        linalg<GPU>::gemm(0, 1, kp__->num_gkvec_row(), kp__->num_gkvec_col(), num_mt_aw, &linalg_const<double_complex>::one(),
                          alm_row.at(memory_t::device, 0, 0, s), alm_row.ld(), alm_col.at(memory_t::device, 0, 0, s), alm_col.ld(), &linalg_const<double_complex>::one(),
                          o__.at(memory_t::device), o__.ld(), omp_get_max_threads());

        linalg<GPU>::gemm(0, 1, kp__->num_gkvec_row(), kp__->num_gkvec_col(), num_mt_aw, &linalg_const<double_complex>::one(),
                          alm_row.at(memory_t::device, 0, 0, s), alm_row.ld(), halm_col.at(memory_t::device, 0, 0, s), halm_col.ld(), &linalg_const<double_complex>::one(),
                          h__.at(memory_t::device), h__.ld(), omp_get_max_threads());
    }

    acc::copyout(h__.at(memory_t::host), h__.ld(), h__.at(memory_t::device), h__.ld(), kp__->num_gkvec_row(), kp__->num_gkvec_col());
    acc::copyout(o__.at(memory_t::host), o__.ld(), o__.at(memory_t::device), o__.ld(), kp__->num_gkvec_row(), kp__->num_gkvec_col());

    // double tval = t1.stop();
    //if (kp__->comm().rank() == 0 && ctx_.control().print_performance_) {
    //    DUMP("effective zgemm performance: %12.6f GFlops",
    //         2 * 8e-9 * kp__->num_gkvec() * kp__->num_gkvec() * unit_cell_.mt_aw_basis_size() / tval);
    //}

    /* add interstitial contributon */
    set_fv_h_o_it(kp__, h__, o__);

    /* setup lo-lo block */
    set_fv_h_o_lo_lo(kp__, h__, o__);

    h__.deallocate(memory_t::device);
    o__.deallocate(memory_t::device);
}
#endif

inline void Hamiltonian::set_fv_h_o_apw_lo(K_point* kp,
                                           Atom_type const& type,
                                           Atom const& atom,
                                           int ia,
                                           mdarray<double_complex, 2>& alm_row, // alm_row comes conjugated
                                           mdarray<double_complex, 2>& alm_col,
                                           mdarray<double_complex, 2>& h,
                                           mdarray<double_complex, 2>& o) const
{
    /* apw-lo block */
    for (int i = 0; i < kp->num_atom_lo_cols(ia); i++) {
        int icol = kp->lo_col(ia, i);
        /* local orbital indices */
        int l     = kp->lo_basis_descriptor_col(icol).l;
        int lm    = kp->lo_basis_descriptor_col(icol).lm;
        int idxrf = kp->lo_basis_descriptor_col(icol).idxrf;
        int order = kp->lo_basis_descriptor_col(icol).order;
        /* loop over apw components */
        for (int j1 = 0; j1 < type.mt_aw_basis_size(); j1++) {
            int lm1    = type.indexb(j1).lm;
            int idxrf1 = type.indexb(j1).idxrf;

            double_complex zsum = atom.radial_integrals_sum_L3<spin_block_t::nm>(idxrf, idxrf1, gaunt_coefs_->gaunt_vector(lm1, lm));

            if (std::abs(zsum) > 1e-14) {
                for (int igkloc = 0; igkloc < kp->num_gkvec_row(); igkloc++) {
                    h(igkloc, kp->num_gkvec_col() + icol) += zsum * alm_row(igkloc, j1);
                }
            }
        }

        for (int order1 = 0; order1 < type.aw_order(l); order1++) {
            int xi1 = type.indexb().index_by_lm_order(lm, order1);
            for (int igkloc = 0; igkloc < kp->num_gkvec_row(); igkloc++) {
                o(igkloc, kp->num_gkvec_col() + icol) += atom.symmetry_class().o_radial_integral(l, order1, order) *
                                                         alm_row(igkloc, xi1);
            }
            if (ctx_.valence_relativity() == relativity_t::iora) {
                int idxrf1 = type.indexr().index_by_l_order(l, order1);
                for (int igkloc = 0; igkloc < kp->num_gkvec_row(); igkloc++) {
                    o(igkloc, kp->num_gkvec_col() + icol) += atom.symmetry_class().o1_radial_integral(idxrf1, idxrf) *
                                                             alm_row(igkloc, xi1);
                }
            }
        }
    }

    std::vector<double_complex> ztmp(kp->num_gkvec_col());
    /* lo-apw block */
    for (int i = 0; i < kp->num_atom_lo_rows(ia); i++) {
        int irow = kp->lo_row(ia, i);
        /* local orbital indices */
        int l     = kp->lo_basis_descriptor_row(irow).l;
        int lm    = kp->lo_basis_descriptor_row(irow).lm;
        int idxrf = kp->lo_basis_descriptor_row(irow).idxrf;
        int order = kp->lo_basis_descriptor_row(irow).order;

        std::fill(ztmp.begin(), ztmp.end(), 0);

        //std::memset(&ztmp[0], 0, kp->num_gkvec_col() * sizeof(double_complex));
        /* loop over apw components */
        for (int j1 = 0; j1 < type.mt_aw_basis_size(); j1++) {
            int lm1    = type.indexb(j1).lm;
            int idxrf1 = type.indexb(j1).idxrf;

            double_complex zsum = atom.radial_integrals_sum_L3<spin_block_t::nm>(idxrf1, idxrf, gaunt_coefs_->gaunt_vector(lm, lm1));

            if (std::abs(zsum) > 1e-14) {
                for (int igkloc = 0; igkloc < kp->num_gkvec_col(); igkloc++) {
                    ztmp[igkloc] += zsum * alm_col(igkloc, j1);
                }
            }
        }

        for (int igkloc = 0; igkloc < kp->num_gkvec_col(); igkloc++) {
            h(irow + kp->num_gkvec_row(), igkloc) += ztmp[igkloc];
        }

        for (int order1 = 0; order1 < type.aw_order(l); order1++) {
            int xi1 = type.indexb().index_by_lm_order(lm, order1);
            for (int igkloc = 0; igkloc < kp->num_gkvec_col(); igkloc++) {
                o(irow + kp->num_gkvec_row(), igkloc) += atom.symmetry_class().o_radial_integral(l, order, order1) *
                                                         alm_col(igkloc, xi1);
            }
            if (ctx_.valence_relativity() == relativity_t::iora) {
                int idxrf1 = type.indexr().index_by_l_order(l, order1);
                for (int igkloc = 0; igkloc < kp->num_gkvec_col(); igkloc++) {
                    o(irow + kp->num_gkvec_row(), igkloc) += atom.symmetry_class().o1_radial_integral(idxrf, idxrf1) *
                                                             alm_col(igkloc, xi1);
                }
            }
        }
    }
}

inline void Hamiltonian::set_fv_h_o_it(K_point* kp,
                                       mdarray<double_complex, 2>& h,
                                       mdarray<double_complex, 2>& o) const
{
    PROFILE("sirius::Hamiltonian::set_fv_h_o_it");

    //#ifdef __PRINT_OBJECT_CHECKSUM
    //double_complex z1 = mdarray<double_complex, 1>(&effective_potential->f_pw(0), ctx_.gvec().num_gvec()).checksum();
    //DUMP("checksum(veff_pw): %18.10f %18.10f", std::real(z1), std::imag(z1));
    //#endif

    double sq_alpha_half = 0.5 * std::pow(speed_of_light, -2);

    #pragma omp parallel for default(shared)
    for (int igk_col = 0; igk_col < kp->num_gkvec_col(); igk_col++) {
        int ig_col          = kp->igk_col(igk_col);
        auto gvec_col       = kp->gkvec().gvec(ig_col);
        auto gkvec_col_cart = kp->gkvec().gkvec_cart<index_domain_t::global>(ig_col);
        for (int igk_row = 0; igk_row < kp->num_gkvec_row(); igk_row++) {
            int ig_row          = kp->igk_row(igk_row);
            auto gvec_row       = kp->gkvec().gvec(ig_row);
            auto gkvec_row_cart = kp->gkvec().gkvec_cart<index_domain_t::global>(ig_row);
            int ig12 = ctx_.gvec().index_g12(gvec_row, gvec_col);
            /* pw kinetic energy */
            double t1 = 0.5 * dot(gkvec_row_cart, gkvec_col_cart);

            h(igk_row, igk_col) += this->potential().veff_pw(ig12);
            o(igk_row, igk_col) += ctx_.theta_pw(ig12);

            if (ctx_.valence_relativity() == relativity_t::none) {
                h(igk_row, igk_col) += t1 * ctx_.theta_pw(ig12);
            } else {
                h(igk_row, igk_col) += t1 * this->potential().rm_inv_pw(ig12);
            }
            if (ctx_.valence_relativity() == relativity_t::iora) {
                o(igk_row, igk_col) += t1 * sq_alpha_half * this->potential().rm2_inv_pw(ig12);
            }
        }
    }
}

inline void Hamiltonian::set_fv_h_o_lo_lo(K_point* kp,
                                          mdarray<double_complex, 2>& h,
                                          mdarray<double_complex, 2>& o) const
{
    PROFILE("sirius::Hamiltonian::set_fv_h_o_lo_lo");

    /* lo-lo block */
    #pragma omp parallel for default(shared)
    for (int icol = 0; icol < kp->num_lo_col(); icol++) {
        int ia     = kp->lo_basis_descriptor_col(icol).ia;
        int lm2    = kp->lo_basis_descriptor_col(icol).lm;
        int idxrf2 = kp->lo_basis_descriptor_col(icol).idxrf;

        for (int irow = 0; irow < kp->num_lo_row(); irow++) {
            /* lo-lo block is diagonal in atom index */
            if (ia == kp->lo_basis_descriptor_row(irow).ia) {
                auto& atom = unit_cell_.atom(ia);
                int lm1    = kp->lo_basis_descriptor_row(irow).lm;
                int idxrf1 = kp->lo_basis_descriptor_row(irow).idxrf;

                h(kp->num_gkvec_row() + irow, kp->num_gkvec_col() + icol) +=
                    atom.radial_integrals_sum_L3<spin_block_t::nm>(idxrf1, idxrf2, gaunt_coefs_->gaunt_vector(lm1, lm2));

                if (lm1 == lm2) {
                    int l      = kp->lo_basis_descriptor_row(irow).l;
                    int order1 = kp->lo_basis_descriptor_row(irow).order;
                    int order2 = kp->lo_basis_descriptor_col(icol).order;
                    o(kp->num_gkvec_row() + irow, kp->num_gkvec_col() + icol) +=
                        atom.symmetry_class().o_radial_integral(l, order1, order2);
                    if (ctx_.valence_relativity() == relativity_t::iora) {
                        int idxrf1 = atom.type().indexr().index_by_l_order(l, order1);
                        int idxrf2 = atom.type().indexr().index_by_l_order(l, order2);
                        o(kp->num_gkvec_row() + irow, kp->num_gkvec_col() + icol) +=
                            atom.symmetry_class().o1_radial_integral(idxrf1, idxrf2);
                    }
                }
            }
        }
    }
}

inline void Hamiltonian::set_o_lo_lo(K_point* kp,
                                     mdarray<double_complex, 2>& o) const
{
    PROFILE("sirius::Hamiltonian::set_o_lo_lo");

    /* lo-lo block */
    #pragma omp parallel for default(shared)
    for (int icol = 0; icol < kp->num_lo_col(); icol++) {
        int ia  = kp->lo_basis_descriptor_col(icol).ia;
        int lm2 = kp->lo_basis_descriptor_col(icol).lm;

        for (int irow = 0; irow < kp->num_lo_row(); irow++) {
            if (ia == kp->lo_basis_descriptor_row(irow).ia) {
                auto& atom = unit_cell_.atom(ia);
                int lm1 = kp->lo_basis_descriptor_row(irow).lm;

                if (lm1 == lm2) {
                    int l      = kp->lo_basis_descriptor_row(irow).l;
                    int order1 = kp->lo_basis_descriptor_row(irow).order;
                    int order2 = kp->lo_basis_descriptor_col(icol).order;
                    o(kp->num_gkvec_row() + irow, kp->num_gkvec_col() + icol) +=
                        atom.symmetry_class().o_radial_integral(l, order1, order2);
                }
            }
        }
    }
}

inline void Hamiltonian::set_o_it(K_point* kp,
                                  mdarray<double_complex, 2>& o) const
{
    PROFILE("sirius::Hamiltonian::set_o_it");

    #pragma omp parallel for default(shared)
    for (int igk_col = 0; igk_col < kp->num_gkvec_col(); igk_col++) {
        auto gvec_col = kp->gkvec().gvec(kp->igk_col(igk_col));
        for (int igk_row = 0; igk_row < kp->num_gkvec_row(); igk_row++) {
            auto gvec_row = kp->gkvec().gvec(kp->igk_row(igk_row));
            int ig12 = ctx_.gvec().index_g12(gvec_row, gvec_col);

            o(igk_row, igk_col) += ctx_.theta_pw(ig12);
        }
    }
}

//template <spin_block_t sblock>
//void Band::set_h_apw_lo(K_point* kp, Atom_type* type, Atom* atom, int ia, mdarray<double_complex, 2>& alm,
//                        mdarray<double_complex, 2>& h)
//{
//    Timer t("sirius::Hamiltonian::set_h_apw_lo");
//
//    int apw_offset_col = kp->apw_offset_col();
//
//    #pragma omp parallel default(shared)
//    {
//        // apw-lo block
//        #pragma omp for
//        for (int i = 0; i < kp->num_atom_lo_cols(ia); i++)
//        {
//            int icol = kp->lo_col(ia, i);
//
//            int lm = kp->gklo_basis_descriptor_col(icol).lm;
//            int idxrf = kp->gklo_basis_descriptor_col(icol).idxrf;
//
//            for (int j1 = 0; j1 < type->mt_aw_basis_size(); j1++)
//            {
//                int lm1 = type->indexb(j1).lm;
//                int idxrf1 = type->indexb(j1).idxrf;
//
//                double_complex zsum = atom->hb_radial_integrals_sum_L3<sblock>(idxrf, idxrf1, gaunt_coefs_->gaunt_vector(lm1, lm));
//
//                if (abs(zsum) > 1e-14)
//                {
//                    for (int igkloc = 0; igkloc < kp->num_gkvec_row(); igkloc++) h(igkloc, icol) += zsum * alm(igkloc, j1);
//                }
//            }
//        }
//    }
//
//    #pragma omp parallel default(shared)
//    {
//        std::vector<double_complex> ztmp(kp->num_gkvec_col());
//        // lo-apw block
//        #pragma omp for
//        for (int i = 0; i < kp->num_atom_lo_rows(ia); i++)
//        {
//            int irow = kp->lo_row(ia, i);
//
//            int lm = kp->gklo_basis_descriptor_row(irow).lm;
//            int idxrf = kp->gklo_basis_descriptor_row(irow).idxrf;
//
//            memset(&ztmp[0], 0, kp->num_gkvec_col() * sizeof(double_complex));
//
//            for (int j1 = 0; j1 < type->mt_aw_basis_size(); j1++)
//            {
//                int lm1 = type->indexb(j1).lm;
//                int idxrf1 = type->indexb(j1).idxrf;
//
//                double_complex zsum = atom->hb_radial_integrals_sum_L3<sblock>(idxrf, idxrf1, gaunt_coefs_->gaunt_vector(lm, lm1));
//
//                if (abs(zsum) > 1e-14)
//                {
//                    for (int igkloc = 0; igkloc < kp->num_gkvec_col(); igkloc++)
//                        ztmp[igkloc] += zsum * conj(alm(apw_offset_col + igkloc, j1));
//                }
//            }
//
//            for (int igkloc = 0; igkloc < kp->num_gkvec_col(); igkloc++) h(irow, igkloc) += ztmp[igkloc];
//        }
//    }
//}

template <spin_block_t sblock>
void Hamiltonian::set_h_it(K_point* kp, Periodic_function<double>* effective_potential,
                           Periodic_function<double>* effective_magnetic_field[3], mdarray<double_complex, 2>& h) const
{
    PROFILE("sirius::Hamiltonian::set_h_it");

    STOP(); // effective potential is now stored in the veff_pw_ auxiliary array. Fix this.

    //#pragma omp parallel for default(shared)
    //for (int igk_col = 0; igk_col < kp->num_gkvec_col(); igk_col++) {
    //    auto gkvec_col_cart = kp->gkvec().gkvec_cart(kp->igk_col(igk_col));
    //    auto gvec_col = kp->gkvec().gvec(kp->igk_col(igk_col));
    //    for (int igk_row = 0; igk_row < kp->num_gkvec_row(); igk_row++) {
    //        auto gkvec_row_cart = kp->gkvec().gkvec_cart(kp->igk_row(igk_row));
    //        auto gvec_row = kp->gkvec().gvec(kp->igk_row(igk_row));

    //        int ig12 = ctx_.gvec().index_g12(gvec_row, gvec_col);
    //
    //        /* pw kinetic energy */
    //        double t1 = 0.5 * (gkvec_row_cart * gkvec_col_cart);
    //
    //        switch (sblock) {
    //            case spin_block_t::nm: {
    //                h(igk_row, igk_col) += (effective_potential->f_pw(ig12) + t1 * ctx_.step_function().theta_pw(ig12));
    //                break;
    //            }
    //            case spin_block_t::uu: {
    //                h(igk_row, igk_col) += (effective_potential->f_pw(ig12) + effective_magnetic_field[0]->f_pw(ig12) +
    //                                        t1 * ctx_.step_function().theta_pw(ig12));
    //                break;
    //            }
    //            case spin_block_t::dd: {
    //                h(igk_row, igk_col) += (effective_potential->f_pw(ig12) - effective_magnetic_field[0]->f_pw(ig12) +
    //                                        t1 * ctx_.step_function().theta_pw(ig12));
    //                break;
    //            }
    //            case spin_block_t::ud: {
    //                h(igk_row, igk_col) += (effective_magnetic_field[1]->f_pw(ig12) -
    //                                        double_complex(0, 1) * effective_magnetic_field[2]->f_pw(ig12));
    //                break;
    //            }
    //            case spin_block_t::du: {
    //                h(igk_row, igk_col) += (effective_magnetic_field[1]->f_pw(ig12) +
    //                                        double_complex(0, 1) * effective_magnetic_field[2]->f_pw(ig12));
    //                break;
    //            }
    //        }
    //    }
    //}
}

template <spin_block_t sblock>
void Hamiltonian::set_h_lo_lo(K_point* kp, mdarray<double_complex, 2>& h) const
{
    PROFILE("sirius::Hamiltonian::set_h_lo_lo");

    /* lo-lo block */
    #pragma omp parallel for default(shared)
    for (int icol = 0; icol < kp->num_lo_col(); icol++) {
        int ia     = kp->lo_basis_descriptor_col(icol).ia;
        int lm2    = kp->lo_basis_descriptor_col(icol).lm;
        int idxrf2 = kp->lo_basis_descriptor_col(icol).idxrf;

        for (int irow = 0; irow < kp->num_lo_row(); irow++) {
            if (ia == kp->lo_basis_descriptor_row(irow).ia) {
                auto& atom = unit_cell_.atom(ia);
                int lm1    = kp->lo_basis_descriptor_row(irow).lm;
                int idxrf1 = kp->lo_basis_descriptor_row(irow).idxrf;

                h(kp->num_gkvec_row() + irow, kp->num_gkvec_col() + icol) +=
                    atom.radial_integrals_sum_L3<sblock>(idxrf1, idxrf2, gaunt_coefs_->gaunt_vector(lm1, lm2));
            }
        }
    }
}

//== template <spin_block_t sblock>
//== void Band::set_h(K_point* kp, Periodic_function<double>* effective_potential,
//==                  Periodic_function<double>* effective_magnetic_field[3], mdarray<double_complex, 2>& h)
//== {
//==     Timer t("sirius::Hamiltonian::set_h");
//==
//==     // index of column apw coefficients in apw array
//==     int apw_offset_col = kp->apw_offset_col();
//==
//==     mdarray<double_complex, 2> alm(kp->num_gkvec_loc(), unit_cell_.max_mt_aw_basis_size());
//==     mdarray<double_complex, 2> halm(kp->num_gkvec_row(), unit_cell_.max_mt_aw_basis_size());
//==
//==     h.zero();
//==
//==     for (int ia = 0; ia < unit_cell_.num_atoms(); ia++)
//==     {
//==         Atom* atom = unit_cell_.atom(ia);
//==         Atom_type* type = atom->type();
//==
//==         // generate conjugated coefficients
//==         kp->generate_matching_coefficients<true>(kp->num_gkvec_loc(), ia, alm);
//==
//==         // apply muffin-tin part to <bra|
//==         apply_hmt_to_apw<sblock>(kp->num_gkvec_row(), ia, alm, halm);
//==
//==         // generate <apw|H|apw> block; |ket> is conjugated, so it is "unconjugated" back
//==         blas<CPU>::gemm(0, 2, kp->num_gkvec_row(), kp->num_gkvec_col(), type->mt_aw_basis_size(), complex_one,
//==                         &halm(0, 0), halm.ld(), &alm(apw_offset_col, 0), alm.ld(), complex_one, &h(0, 0), h.ld());
//==
//==         // setup apw-lo blocks
//==         set_h_apw_lo<sblock>(kp, type, atom, ia, alm, h);
//==     } //ia
//==
//==     set_h_it<sblock>(kp, effective_potential, effective_magnetic_field, h);
//==
//==     set_h_lo_lo<sblock>(kp, h);
//==
//==     alm.deallocate();
//==     halm.deallocate();
//== }

//void Band::set_o_apw_lo(K_point* kp, Atom_type* type, Atom* atom, int ia, mdarray<double_complex, 2>& alm,
//                        mdarray<double_complex, 2>& o)
//{
//    Timer t("sirius::Hamiltonian::set_o_apw_lo");
//
//    int apw_offset_col = kp->apw_offset_col();
//
//    // apw-lo block
//    for (int i = 0; i < kp->num_atom_lo_cols(ia); i++)
//    {
//        int icol = kp->lo_col(ia, i);
//
//        int l = kp->gklo_basis_descriptor_col(icol).l;
//        int lm = kp->gklo_basis_descriptor_col(icol).lm;
//        int order = kp->gklo_basis_descriptor_col(icol).order;
//
//        for (int order1 = 0; order1 < (int)type->aw_descriptor(l).size(); order1++)
//        {
//            for (int igkloc = 0; igkloc < kp->num_gkvec_row(); igkloc++)
//            {
//                o(igkloc, icol) += atom->symmetry_class()->o_radial_integral(l, order1, order) *
//                                   alm(igkloc, type->indexb_by_lm_order(lm, order1));
//            }
//        }
//    }
//
//    std::vector<double_complex> ztmp(kp->num_gkvec_col());
//    // lo-apw block
//    for (int i = 0; i < kp->num_atom_lo_rows(ia); i++)
//    {
//        int irow = kp->lo_row(ia, i);
//
//        int l = kp->gklo_basis_descriptor_row(irow).l;
//        int lm = kp->gklo_basis_descriptor_row(irow).lm;
//        int order = kp->gklo_basis_descriptor_row(irow).order;
//
//        for (int order1 = 0; order1 < (int)type->aw_descriptor(l).size(); order1++)
//        {
//            for (int igkloc = 0; igkloc < kp->num_gkvec_col(); igkloc++)
//            {
//                o(irow, igkloc) += atom->symmetry_class()->o_radial_integral(l, order, order1) *
//                                   conj(alm(apw_offset_col + igkloc, type->indexb_by_lm_order(lm, order1)));
//            }
//        }
//    }
//}

//== void Band::set_o(K_point* kp, mdarray<double_complex, 2>& o)
//== {
//==     Timer t("sirius::Hamiltonian::set_o");
//==
//==     // index of column apw coefficients in apw array
//==     int apw_offset_col = kp->apw_offset_col();
//==
//==     mdarray<double_complex, 2> alm(kp->num_gkvec_loc(), unit_cell_.max_mt_aw_basis_size());
//==     o.zero();
//==
//==     double_complex zone(1, 0);
//==
//==     for (int ia = 0; ia < unit_cell_.num_atoms(); ia++)
//==     {
//==         Atom* atom = unit_cell_.atom(ia);
//==         Atom_type* type = atom->type();
//==
//==         // generate conjugated coefficients
//==         kp->generate_matching_coefficients<true>(kp->num_gkvec_loc(), ia, alm);
//==
//==         // generate <apw|apw> block; |ket> is conjugated, so it is "unconjugated" back
//==         blas<CPU>::gemm(0, 2, kp->num_gkvec_row(), kp->num_gkvec_col(), type->mt_aw_basis_size(), zone,
//==                         &alm(0, 0), alm.ld(), &alm(apw_offset_col, 0), alm.ld(), zone, &o(0, 0), o.ld());
//==
//==         // setup apw-lo blocks
//==         set_o_apw_lo(kp, type, atom, ia, alm, o);
//==     } //ia
//==
//==     set_o_it(kp, o);
//==
//==     set_o_lo_lo(kp, o);
//==
//==     alm.deallocate();
//== }
