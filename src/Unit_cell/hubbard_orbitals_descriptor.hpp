// Copyright (c) 2013-2018 Mathieu Taillefumier, Anton Kozhevnikov, Thomas Schulthess
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

/** \file hubbard_orbitals_descriptor.hpp
 *
 *  \brief Contains a descriptor class for Hubbard orbitals.
 */

#ifndef __HUBBARD_ORBITALS_DESCRIPTOR_HPP__
#define __HUBBARD_ORBITALS_DESCRIPTOR_HPP__

namespace sirius {

/// Structure containing all informations about a specific hubbard orbital (including the index of the radial function).
class hubbard_orbital_descriptor
{
  private:
    /// Principal quantum number of atomic orbital.
    int n_{-1};
    /// Orbital quantum number of atomic orbital.
    int l_{-1};
    /// Orbital Occupancy.
    double occupancy_{-1.0};

    int radial_orbital_index_{-1};

    double hubbard_J_{0.0};
    double hubbard_U_{0.0};

    /// Different hubbard coefficients.
    /** s: U = hubbard_coefficients_[0]
        p: U = hubbard_coefficients_[0], J = hubbard_coefficients_[1]
        d: U = hubbard_coefficients_[0], J = hubbard_coefficients_[1],  B  = hubbard_coefficients_[2]
        f: U = hubbard_coefficients_[0], J = hubbard_coefficients_[1],  E2 = hubbard_coefficients_[2], E3 =
        hubbard_coefficients_[3]
        hubbard_coefficients[4] = U_alpha
        hubbard_coefficients[5] = U_beta */
    double hubbard_coefficients_[4];

    mdarray<double, 4> hubbard_matrix_;

    // simplifed hubbard theory
    double hubbard_alpha_{0.0};
    double hubbard_beta_{0.0};
    double hubbard_J0_{0.0};

  private:
    inline void calculate_ak_coefficients(mdarray<double, 5>& ak)
    {
        // compute the ak coefficients appearing in the general treatment of
        // hubbard corrections.  expression taken from Liechtenstein {\it et
        // al}, PRB 52, R5467 (1995)

        // Note that for consistency, the ak are calculated with complex
        // harmonics in the gaunt coefficients <R_lm|Y_l'm'|R_l''m''>.
        // we need to keep it that way because of the hubbard potential
        // With a spherical one it does not really matter-
        ak.zero();

        for (int m1 = -l_; m1 <= l_; m1++) {
            for (int m2 = -l_; m2 <= l_; m2++) {
                for (int m3 = -l_; m3 <= l_; m3++) {
                    for (int m4 = -l_; m4 <= l_; m4++) {
                        for (int k = 0; k < 2 * l_; k += 2) {
                            double sum = 0.0;
                            for (int q = -k; q <= k; q++) {
                                sum += SHT::gaunt_rlm_ylm_rlm(l_, k, l_, m1, q, m2) *
                                       SHT::gaunt_rlm_ylm_rlm(l_, k, l_, m3, q, m4);
                            }
                            // hmmm according to PRB 52, R5467 it is 4
                            // \pi/(2 k + 1) -> 4 \pi / (4 * k + 1) because
                            // I only consider a_{k=0} a_{k=2}, a_{k=4}
                            ak(k / 2, m1 + l_, m2 + l_, m3 + l_,
                               m4 + l_) = 4.0 * sum * M_PI / static_cast<double>(2 * k + 1);
                        }
                    }
                }
            }
        }
    }

    /// this function computes the matrix elements of the orbital part of
    /// the electron-electron interactions. we effectively compute

    /// \f[ u(m,m'',m',m''') = \left<m,m''|V_{e-e}|m',m'''\right> \sum_k
    /// a_k(m,m',m'',m''') F_k \f] where the F_k are calculated for real
    /// spherical harmonics

    inline void compute_hubbard_matrix()
    {
        this->hubbard_matrix_ = mdarray<double, 4>(2 * l_ + 1, 2 * l_ + 1,
                                                   2 * l_ + 1, 2 * l_ + 1);
        mdarray<double, 5> ak(l_, 2 * l_ + 1, 2 * l_ + 1,
                              2 * l_ + 1, 2 * l_ + 1);
        std::vector<double> F(4);
        hubbard_F_coefficients(&F[0]);
        calculate_ak_coefficients(ak);

        // the indices are rotated around

        // <m, m |vee| m'', m'''> = hubbard_matrix(m, m'', m', m''')
        this->hubbard_matrix_.zero();
        for (int m1 = 0; m1 < 2 * l_ + 1; m1++) {
            for (int m2 = 0; m2 < 2 * l_ + 1; m2++) {
                for (int m3 = 0; m3 < 2 * l_ + 1; m3++) {
                    for (int m4 = 0; m4 < 2 * l_ + 1; m4++) {
                        for (int k = 0; k < l_; k++)
                            this->hubbard_matrix(m1, m2, m3, m4) += ak(k, m1, m3, m2, m4) * F[k];
                    }
                }
            }
        }
    }

    void hubbard_F_coefficients(double* F)
    {
        F[0] = Hubbard_U();

        switch (l_) {
            case 0: {
                F[1] = Hubbard_J();
                break;
            }
            case 1: {
                F[1] = 5.0 * Hubbard_J();
                break;
            }
            case 2: {
                F[1] = 5.0 * Hubbard_J() + 31.5 * Hubbard_B();
                F[2] = 9.0 * Hubbard_J() - 31.5 * Hubbard_B();
                break;
            }
            case 3: {
                F[1] = (225.0 / 54.0) * Hubbard_J() + (32175.0 / 42.0) * Hubbard_E2() + (2475.0 / 42.0) * Hubbard_E3();
                F[2] = 11.0 * Hubbard_J() - (141570.0 / 77.0) * Hubbard_E2() + (4356.0 / 77.0) * Hubbard_E3();
                F[3] = (7361.640 / 594.0) * Hubbard_J() + (36808.20 / 66.0) * Hubbard_E2() - 111.54 * Hubbard_E3();
                break;
            }
            default: {
                std::stringstream s;
                s << "Hubbard correction not implemented for l > 3\n"
                  << "  current l: " << l_;
                TERMINATE(s);
                break;
            }
        }
    }

  public:
    inline int l() const
    {
        return l_;
    }

    inline int n() const
    {
        return n_;
    }

    inline double hubbard_matrix(const int m1, const int m2, const int m3, const int m4) const
    {
        return hubbard_matrix_(m1, m2, m3, m4);
    }

    inline double& hubbard_matrix(const int m1, const int m2, const int m3, const int m4)
    {
        return hubbard_matrix_(m1, m2, m3, m4);
    }

    inline double Hubbard_J0() const
    {
        return hubbard_J0_;
    }

    inline double Hubbard_U() const
    {
        return hubbard_U_;
    }

    inline double Hubbard_J() const
    {
        return hubbard_J_;
    }

    inline double Hubbard_U_minus_J() const
    {
        return Hubbard_U() - Hubbard_J();
    }

    inline double Hubbard_B() const
    {
        return hubbard_coefficients_[2];
    }

    inline double Hubbard_E2() const
    {
        return hubbard_coefficients_[2];
    }

    inline double Hubbard_E3() const
    {
        return hubbard_coefficients_[3];
    }

    inline double Hubbard_alpha() const
    {
        return hubbard_alpha_;
    }

    inline double Hubbard_beta() const
    {
        return hubbard_beta_;
    }

    inline double occupancy() const
    {
        return occupancy_;
    }

    inline int rindex() const
    {
        return radial_orbital_index_;
    }

    void initialize_hubbard_matrix()
    {
        mdarray<double, 5> ak(l_, 2 * l_ + 1, 2 * l_ + 1,
                              2 * l_ + 1, 2 * l_ + 1);
        std::vector<double> F(4);
        hubbard_F_coefficients(&F[0]);
        calculate_ak_coefficients(ak);

        this->hubbard_matrix_ = mdarray<double, 4>(2 * l_ + 1, 2 * l_ + 1,
                                                   2 * l_ + 1, 2 * l_ + 1);
        // the indices are rotated around

        // <m, m |vee| m'', m'''> = hubbard_matrix(m, m'', m', m''')
        this->hubbard_matrix_.zero();
        for (int m1 = 0; m1 < 2 * l_ + 1; m1++) {
            for (int m2 = 0; m2 < 2 * l_ + 1; m2++) {
                for (int m3 = 0; m3 < 2 * l_ + 1; m3++) {
                    for (int m4 = 0; m4 < 2 * l_ + 1; m4++) {
                        for (int k = 0; k < l_; k++)
                            this->hubbard_matrix(m1, m2, m3, m4) += ak(k, m1, m3, m2, m4) * F[k];
                    }
                }
            }
        }
    }

  public:
    hubbard_orbital_descriptor()
    {
    }

    hubbard_orbital_descriptor(const int n__, const int l__, const int orbital_index__, const double occ__,
                               const double J__, const double U__, const double* hub_coef__, const double alpha__,
                               const double beta__, const double J0__)
        : n_(n__)
        , l_(l__)
        , occupancy_(occ__)
        , radial_orbital_index_(orbital_index__)
        , hubbard_J_(J__)
        , hubbard_U_(U__)
        , hubbard_alpha_(alpha__)
        , hubbard_beta_(beta__)
        , hubbard_J0_(J0__)
    {
        if (hub_coef__) {
            for (int s = 0; s < 4; s++) {
                hubbard_coefficients_[s] = hub_coef__[s];
            }

            initialize_hubbard_matrix();
        }
    }

    ~hubbard_orbital_descriptor()
    {
    }

    hubbard_orbital_descriptor(hubbard_orbital_descriptor&& src)
        : n_(src.n_)
        , l_(src.l_)
        , occupancy_(src.occupancy_)
        , radial_orbital_index_(src.radial_orbital_index_)
        , hubbard_J_(src.hubbard_J_)
        , hubbard_U_(src.hubbard_U_)
        , hubbard_alpha_(src.hubbard_alpha_)
        , hubbard_beta_(src.hubbard_beta_)
    {
        hubbard_matrix_ = std::move(src.hubbard_matrix_);
        for (int s = 0; s < 4; s++) {
            hubbard_coefficients_[s] = src.hubbard_coefficients_[s];
        }
    }
};

} // namespace sirius

#endif
