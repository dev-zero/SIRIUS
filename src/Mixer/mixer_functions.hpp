// Copyright (c) 2013-2018 Anton Kozhevnikov, Thomas Schulthess
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

#ifndef __MIXER_FUNCTIONS_HPP__
#define __MIXER_FUNCTIONS_HPP__

#include "Mixer/mixer.hpp"

namespace sirius {

namespace mixer {
inline MixerFunctionProperties<Periodic_function<double>> full_potential_periodic_function_property(bool local)
{
    auto inner_prod_func = [](const Periodic_function<double>& x, const Periodic_function<double>& y) -> double {
        double result = 0.0;
        for (std::size_t i = 0; i < x.f_rg().size(); ++i) {
            result += std::real(std::conj(x.f_rg(i)) * y.f_rg(i));
        }

        for (int ialoc = 0; ialoc < x.ctx().unit_cell().spl_num_atoms().local_size(); ialoc++) {
            auto& x_f_mt = const_cast<Spheric_function<function_domain_t::spectral, double>&>(x.f_mt(ialoc));
            auto& y_f_mt = const_cast<Spheric_function<function_domain_t::spectral, double>&>(y.f_mt(ialoc));
            for (int i = 0; i < static_cast<int>(x.f_mt(ialoc).size()); i++) {
                result += x_f_mt[i] * y_f_mt[i];
            }
        }
        return result;
    };

    auto scal_function = [](double alpha, Periodic_function<double>& x) -> void {
        for (std::size_t i = 0; i < x.f_rg().size(); ++i) {
            x.f_rg(i) *= alpha;
        }
        for (int ialoc = 0; ialoc < x.ctx().unit_cell().spl_num_atoms().local_size(); ialoc++) {
            auto& x_f_mt = const_cast<Spheric_function<function_domain_t::spectral, double>&>(x.f_mt(ialoc));
            for (int i = 0; i < static_cast<int>(x.f_mt(ialoc).size()); i++) {
                x_f_mt[i] *= alpha;
            }
        }
    };

    auto copy_function = [](const Periodic_function<double>& x, Periodic_function<double>& y) -> void {
        for (std::size_t i = 0; i < x.f_rg().size(); ++i) {
            y.f_rg(i) = x.f_rg(i);
        }
        for (int ialoc = 0; ialoc < x.ctx().unit_cell().spl_num_atoms().local_size(); ialoc++) {
            auto& x_f_mt = const_cast<Spheric_function<function_domain_t::spectral, double>&>(x.f_mt(ialoc));
            auto& y_f_mt = const_cast<Spheric_function<function_domain_t::spectral, double>&>(y.f_mt(ialoc));
            for (int i = 0; i < static_cast<int>(x.f_mt(ialoc).size()); i++) {
                y_f_mt[i] = x_f_mt[i];
            }
        }
    };

    auto axpy_function = [](double alpha, const Periodic_function<double>& x, Periodic_function<double>& y) -> void {
        for (std::size_t i = 0; i < x.f_rg().size(); ++i) {
            y.f_rg(i) += alpha * x.f_rg(i);
        }
        for (int ialoc = 0; ialoc < x.ctx().unit_cell().spl_num_atoms().local_size(); ialoc++) {
            auto& x_f_mt = const_cast<Spheric_function<function_domain_t::spectral, double>&>(x.f_mt(ialoc));
            auto& y_f_mt = const_cast<Spheric_function<function_domain_t::spectral, double>&>(y.f_mt(ialoc));
            for (int i = 0; i < static_cast<int>(x.f_mt(ialoc).size()); i++) {
                y_f_mt[i] += alpha * x_f_mt[i];
            }
        }
    };

    return MixerFunctionProperties<Periodic_function<double>>(local, inner_prod_func, scal_function, copy_function,
                                                              axpy_function);
}

inline MixerFunctionProperties<Periodic_function<double>> pseudo_potential_periodic_function_property(bool local)
{

    auto inner_prod_func = [](const Periodic_function<double>& x, const Periodic_function<double>& y) -> double {
        double result      = 0.0;
        const auto& x_f_pw = x.f_pw_local();
        const auto& y_f_pw = y.f_pw_local();
        assert(x_f_pw.size() == y_f_pw.size());
        for (std::size_t i = 0; i < x_f_pw.size(); ++i) {
            result += std::real(std::conj(x_f_pw[i]) * y_f_pw[i]);
        }
        return result;
    };

    auto scal_function = [](double alpha, Periodic_function<double>& x) -> void {
        auto& x_f_pw = x.f_pw_local();
        for (std::size_t i = 0; i < x_f_pw.size(); ++i) {
            x_f_pw[i] *= alpha;
        }
    };

    auto copy_function = [](const Periodic_function<double>& x, Periodic_function<double>& y) -> void {
        const auto& x_f_pw = x.f_pw_local();
        auto& y_f_pw       = y.f_pw_local();
        assert(x_f_pw.size() == y_f_pw.size());
        for (std::size_t i = 0; i < x_f_pw.size(); ++i) {
            y_f_pw[i] = x_f_pw[i];
        }
    };

    auto axpy_function = [](double alpha, const Periodic_function<double>& x, Periodic_function<double>& y) -> void {
        const auto& x_f_pw = x.f_pw_local();
        auto& y_f_pw       = y.f_pw_local();
        assert(x_f_pw.size() == y_f_pw.size());
        for (std::size_t i = 0; i < x_f_pw.size(); ++i) {
            y_f_pw[i] += alpha * x_f_pw[i];
        }
    };

    return MixerFunctionProperties<Periodic_function<double>>(local, inner_prod_func, scal_function, copy_function,
                                                              axpy_function);
}

inline MixerFunctionProperties<mdarray<double_complex, 4>> density_function_property(bool local)
{

    auto inner_prod_func = [](const mdarray<double_complex, 4>& x, const mdarray<double_complex, 4>& y) -> double {
        // do not contribute to mixing
        return 0.0;
    };

    auto scal_function = [](double alpha, mdarray<double_complex, 4>& x) -> void {
        for (std::size_t i = 0; i < x.size(); ++i) {
            x[i] *= alpha;
        }
    };

    auto copy_function = [](const mdarray<double_complex, 4>& x, mdarray<double_complex, 4>& y) -> void {
        for (std::size_t i = 0; i < x.size(); ++i) {
            y[i] = x[i];
        }
    };

    auto axpy_function = [](double alpha, const mdarray<double_complex, 4>& x, mdarray<double_complex, 4>& y) -> void {
        for (std::size_t i = 0; i < x.size(); ++i) {
            y[i] += alpha * x[i];
        }
    };

    return MixerFunctionProperties<mdarray<double_complex, 4>>(local, inner_prod_func, scal_function, copy_function,
                                                               axpy_function);
}

} // namespace mixer

} // namespace sirius

#endif
