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

/** \file atom_type_base.hpp
 *
 *  \brief Contains declaration and implementation of sirius::Atom_type_base class.
 */

#ifndef __ATOM_TYPE_BASE_HPP__
#define __ATOM_TYPE_BASE_HPP__

#include "atomic_data.hpp"
#include "radial_grid.hpp"

namespace sirius {

/// Base class for sirius::Atom_type and sirius::Free_atom classes.
class Atom_type_base
{
  protected:
    /// Nucleus charge or pseudocharge, treated as positive(!) integer.
    int zn_{0};

    /// Chemical element symbol.
    std::string symbol_;

    /// Chemical element name.
    std::string name_;

    /// Atom mass.
    double mass_{0};

    /// List of atomic levels.
    std::vector<atomic_level_descriptor> atomic_levels_;

    /// Number of core electrons.
    double num_core_electrons_{0};

    /// Number of valence electrons.
    double num_valence_electrons_{0};

    /* forbid copy constructor */
    Atom_type_base(Atom_type_base const& src) = delete;

    /* forbid assignment operator */
    Atom_type_base& operator=(Atom_type_base& src) = delete;

    /// Density of a free atom.
    Spline<double> free_atom_density_spline_;

    /// Density of a free atom as read from the input file.
    /** Does not contain 4 Pi and r^2 prefactors. */
    std::vector<double> free_atom_density_;

    /// Radial grid of a free atom.
    Radial_grid<double> free_atom_radial_grid_;

  private:
    void init()
    {
        /* add valence levels to the list of atom's levels */
        for (auto& e : atomic_conf[zn_ - 1]) {
            /* check if this level is already in the list */
            bool in_list{false};
            for (auto& c : atomic_levels_) {
                if (c.n == e.n && c.l == e.l && c.k == e.k) {
                    in_list = true;
                    break;
                }
            }
            if (!in_list) {
                auto level = e;
                level.core = false;
                atomic_levels_.push_back(level);
            }
        }
        ///* get the number of core electrons */
        //for (auto& e : atomic_levels_) {
        //    if (e.core) {
        //        num_core_electrons_ += e.occupancy;
        //    }
        //}

        ///* get number of valence electrons */
        //num_valence_electrons_ = zn_ - num_core_electrons_;

        free_atom_radial_grid_ = Radial_grid_exp<double>(2000 + 150 * zn(), 1e-7, 20.0 + 0.25 * zn(), 1.0);
    }

  public:
    Atom_type_base(int zn__)
        : zn_(zn__)
        , symbol_(atomic_symb[zn_ - 1])
        , name_(atomic_name[zn_ - 1])
    {
        init();
    }

    Atom_type_base(std::string symbol__)
        : zn_(atomic_zn.at(symbol__))
        , symbol_(symbol__)
        , name_(atomic_name[zn_ - 1])
    {
        init();
    }

    //Atom_type(Simulation_parameters const&                parameters__,
    //          std::string                                 symbol__,
    //          std::string                                 name__,
    //          int                                         zn__,
    //          double                                      mass__,
    //          std::vector<atomic_level_descriptor> const& levels__)
    //    : parameters_(parameters__)
    //    , symbol_(symbol__)
    //    , name_(name__)
    //    , zn_(zn__)
    //    , mass_(mass__)
    //    , atomic_levels_(levels__)
    //{
    //}

    //Atom_type(Simulation_parameters const& parameters__, int id__, std::string label__, std::string file_name__)
    //    : parameters_(parameters__)
    //    , id_(id__)
    //    , label_(label__)
    //    , file_name_(file_name__)
    //{
    //}

    //Atom_type(Atom_type&& src) = default;

    //inline void init(int offset_lo__);

    //inline void set_radial_grid(radial_grid_t grid_type__, int num_points__, double rmin__, double rmax__, double p__)
    //{
    //    radial_grid_ = Radial_grid_factory<double>(grid_type__, num_points__, rmin__, rmax__, p__);
    //    if (parameters_.processing_unit() == GPU) {
    //        radial_grid_.copy_to_device();
    //    }
    //}

    //inline void set_radial_grid(int num_points__, double const* points__)
    //{
    //    radial_grid_ = Radial_grid_ext<double>(num_points__, points__);
    //    if (parameters_.processing_unit() == GPU) {
    //        radial_grid_.copy_to_device();
    //    }
    //}

    ///// Add augmented-wave descriptor.
    //inline void add_aw_descriptor(int n, int l, double enu, int dme, int auto_enu)
    //{
    //    if (static_cast<int>(aw_descriptors_.size()) < (l + 1)) {
    //        aw_descriptors_.resize(l + 1, radial_solution_descriptor_set());
    //    }

    //    radial_solution_descriptor rsd;

    //    rsd.n = n;
    //    if (n == -1) {
    //        /* default principal quantum number value for any l */
    //        rsd.n = l + 1;
    //        for (int ist = 0; ist < num_atomic_levels(); ist++) {
    //            /* take next level after the core */
    //            if (atomic_level(ist).core && atomic_level(ist).l == l) {
    //                rsd.n = atomic_level(ist).n + 1;
    //            }
    //        }
    //    }

    //    rsd.l        = l;
    //    rsd.dme      = dme;
    //    rsd.enu      = enu;
    //    rsd.auto_enu = auto_enu;
    //    aw_descriptors_[l].push_back(rsd);
    //}

    ///// Add local orbital descriptor
    //inline void add_lo_descriptor(int ilo, int n, int l, double enu, int dme, int auto_enu)
    //{
    //    if ((int)lo_descriptors_.size() == ilo) {
    //        lo_descriptors_.push_back(local_orbital_descriptor());
    //        lo_descriptors_[ilo].l = l;
    //    } else {
    //        if (l != lo_descriptors_[ilo].l) {
    //            std::stringstream s;
    //            s << "wrong angular quantum number" << std::endl
    //              << "atom type id: " << id() << " (" << symbol_ << ")" << std::endl
    //              << "idxlo: " << ilo << std::endl
    //              << "n: " << l << std::endl
    //              << "l: " << n << std::endl
    //              << "expected l: " << lo_descriptors_[ilo].l << std::endl;
    //            TERMINATE(s);
    //        }
    //    }

    //    radial_solution_descriptor rsd;

    //    rsd.n = n;
    //    if (n == -1) {
    //        /* default value for any l */
    //        rsd.n = l + 1;
    //        for (int ist = 0; ist < num_atomic_levels(); ist++) {
    //            if (atomic_level(ist).core && atomic_level(ist).l == l) {
    //                /* take next level after the core */
    //                rsd.n = atomic_level(ist).n + 1;
    //            }
    //        }
    //    }

    //    rsd.l        = l;
    //    rsd.dme      = dme;
    //    rsd.enu      = enu;
    //    rsd.auto_enu = auto_enu;
    //    lo_descriptors_[ilo].rsd_set.push_back(rsd);
    //}

    //inline void add_lo_descriptor(local_orbital_descriptor const& lod__)
    //{
    //    lo_descriptors_.push_back(lod__);
    //}

    //inline void add_ps_atomic_wf(int n__, int l__, std::vector<double> f__)
    //{
    //    Spline<double> s(radial_grid_, f__);
    //    ps_atomic_wfs_.push_back(std::move(std::make_pair(l__, std::move(s))));
    //    ps_atomic_wf_level_.push_back(n__);
    //}

    //inline void add_ps_atomic_wf(int n__, int l__, std::vector<double> f__, double occ_)
    //{
    //    Spline<double> s(radial_grid_, f__);
    //    ps_atomic_wfs_.push_back(std::move(std::make_pair(l__, std::move(s))));
    //    ps_atomic_wf_occ_.push_back(occ_);
    //    ps_atomic_wf_level_.push_back(n__);
    //}

    //std::pair<int, Spline<double>> const& ps_atomic_wf(int idx__) const
    //{
    //    return ps_atomic_wfs_[idx__];
    //}

    //inline int lmax_ps_atomic_wf() const
    //{
    //    int lmax{-1};
    //    for (auto& e: ps_atomic_wfs_) {
    //        lmax = std::max(lmax, std::abs(e.first));
    //    }
    //    return lmax;
    //}

    //inline int num_ps_atomic_wf() const
    //{
    //    return static_cast<int>(ps_atomic_wfs_.size());
    //}

    //inline std::vector<double> const& ps_atomic_wf_occ() const
    //{
    //    return ps_atomic_wf_occ_;
    //}

    //inline std::vector<double>& ps_atomic_wf_occ(std::vector<double> inp__)
    //{
    //    ps_atomic_wf_occ_.clear();
    //    ps_atomic_wf_occ_ = inp__;
    //    return ps_atomic_wf_occ_;
    //}

    ///// Add a radial function of beta-projector to a list of functions.
    //inline void add_beta_radial_function(int l__, std::vector<double> beta__)
    //{
    //    if (augment_) {
    //        TERMINATE("can't add more beta projectors");
    //    }
    //    Spline<double> s(radial_grid_, beta__);
    //    beta_radial_functions_.push_back(std::move(std::make_pair(l__, std::move(s))));
    //}

    ///// Return a radial beta functions.
    //inline Spline<double> const& beta_radial_function(int idxrf__) const
    //{
    //    return beta_radial_functions_[idxrf__].second;
    //}

    ///// Maximum orbital quantum number between all beta-projector radial functions.
    //inline int lmax_beta() const
    //{
    //    int lmax{-1};

    //    /* need to take |l| since the total angular momentum is encoded in the sign of l */
    //    for (auto& e: beta_radial_functions_) {
    //        lmax = std::max(lmax, std::abs(e.first));
    //    }
    //    return lmax;
    //}

    ///// Number of beta-radial functions.
    //inline int num_beta_radial_functions() const
    //{
    //    return static_cast<int>(beta_radial_functions_.size());
    //}

    //inline void add_q_radial_function(int idxrf1__, int idxrf2__, int l__, std::vector<double> qrf__)
    //{
    //    /* sanity check */
    //    if (l__ > 2 * lmax_beta()) {
    //        std::stringstream s;
    //        s << "wrong l for Q radial functions of atom type " << label_ << std::endl
    //          << "current l: " << l__ << std::endl
    //          << "lmax_beta: " << lmax_beta() << std::endl
    //          << "maximum allowed l: " << 2 * lmax_beta();

    //        TERMINATE(s);
    //    }

    //    if (!augment_) {
    //        /* once we add a Q-radial function, we need to augment the charge */
    //        augment_ = true;
    //        /* number of radial beta-functions */
    //        int nbrf = num_beta_radial_functions();
    //        q_radial_functions_l_ = mdarray<Spline<double>, 2>(nbrf * (nbrf + 1) / 2, 2 * lmax_beta() + 1);

    //        for (int l = 0; l <= 2 * lmax_beta(); l++) {
    //            for (int idx = 0; idx < nbrf * (nbrf + 1) / 2; idx++) {
    //                q_radial_functions_l_(idx, l) = Spline<double>(radial_grid_);
    //            }
    //        }
    //    }

    //    int ijv = utils::packed_index(idxrf1__, idxrf2__);
    //    q_radial_functions_l_(ijv, l__) = Spline<double>(radial_grid_, qrf__);
    //}

    //inline bool augment() const
    //{
    //    return augment_;
    //}

    //inline std::vector<double>& local_potential(std::vector<double> vloc__)
    //{
    //    local_potential_ = vloc__;
    //    return local_potential_;
    //}

    //inline std::vector<double> const& local_potential() const
    //{
    //    return local_potential_;
    //}

    //inline std::vector<double>& ps_core_charge_density(std::vector<double> ps_core__)
    //{
    //    ps_core_charge_density_ = ps_core__;
    //    return ps_core_charge_density_;
    //}

    //inline std::vector<double> const& ps_core_charge_density() const
    //{
    //    return ps_core_charge_density_;
    //}

    //inline std::vector<double>& ps_total_charge_density(std::vector<double> ps_dens__)
    //{
    //    ps_total_charge_density_ = ps_dens__;
    //    return ps_total_charge_density_;
    //}

    //inline std::vector<double> const& ps_total_charge_density() const
    //{
    //    return ps_total_charge_density_;
    //}

    ///// Add all-electron PAW wave-function.
    //inline void add_ae_paw_wf(std::vector<double> f__)
    //{
    //    ae_paw_wfs_.push_back(f__);
    //}

    ///// Get all-electron PAW wave-function.
    //inline std::vector<double> const& ae_paw_wf(int i__) const
    //{
    //    return ae_paw_wfs_[i__];
    //}

    ///// Get the number of all-electron PAW wave-functions.
    //inline int num_ae_paw_wf() const
    //{
    //    return static_cast<int>(ae_paw_wfs_.size());
    //}

    //inline void add_ps_paw_wf(std::vector<double> f__)
    //{
    //    ps_paw_wfs_.push_back(f__);
    //}

    //inline std::vector<double> const& ps_paw_wf(int i__) const
    //{
    //    return ps_paw_wfs_[i__];
    //}

    //inline int num_ps_paw_wf() const
    //{
    //    return static_cast<int>(ps_paw_wfs_.size());
    //}

    //inline mdarray<double, 2> const& ae_paw_wfs_array() const
    //{
    //    return ae_paw_wfs_array_;
    //}

    //inline mdarray<double, 2> const& ps_paw_wfs_array() const
    //{
    //    return ps_paw_wfs_array_;
    //}

    //inline std::vector<double> const& paw_ae_core_charge_density() const
    //{
    //    return paw_ae_core_charge_density_;
    //}

    //inline std::vector<double>& paw_ae_core_charge_density(std::vector<double> inp__)
    //{
    //    paw_ae_core_charge_density_ = inp__;
    //    return paw_ae_core_charge_density_;
    //}

    //inline std::vector<double> const& paw_wf_occ() const
    //{
    //    return paw_wf_occ_;
    //}

    //inline std::vector<double>& paw_wf_occ(std::vector<double> inp__)
    //{
    //    paw_wf_occ_ = inp__;
    //    return paw_wf_occ_;
    //}

    ///// Initialize the free atom density (smooth or true).
    //inline void init_free_atom_density(bool smooth);

    //void add_hubbard_orbital(int n,
    //                         int l,
    //                         double occ,
    //                         double U,
    //                         double J,
    //                         const double *hub_coef__,
    //                         double alpha__,
    //                         double beta__,
    //                         double J0__)
    //{
    //    for (int s = 0; s < static_cast<int>(ps_atomic_wf_level_.size()); s++) {
    //        if (ps_atomic_wf_level_[s] >= 0) {
    //            if ((ps_atomic_wf_level_[s] ==  n) && (std::abs(ps_atomic_wf(s).first) == l)) {
    //                hubbard_orbital_descriptor_ hub(n, l, s, occ, J, U, hub_coef__, alpha__, beta__, J0__);
    //                hubbard_orbitals_.push_back(std::move(hub));

    //                // we nedd to consider the case where spin orbit
    //                // coupling is included. if so is included then we need
    //                // to search for its partner with same n, l, but
    //                // different j. if not we can stop the for loop

    //                if (!this->spin_orbit_coupling_)
    //                    break;
    //            }
    //        } else {

    //            auto &wfc = ps_atomic_wfs_[s];
    //            // we do a search per angular momentum
    //            // we pick the first atomic wave function we
    //            // find with the right l. It is to deal with
    //            // codes that do not store all info about wave
    //            // functions.
    //            if (std::abs(wfc.first) == l) {
    //                hubbard_orbital_descriptor_ hub(n, l, s, occ, J, U, hub_coef__, alpha__, beta__, J0__);
    //                hubbard_orbitals_.push_back(std::move(hub));

    //                if (!this->spin_orbit_coupling_)
    //                    break;
    //            }
    //        }
    //    }
    //}

    //inline int number_of_hubbard_channels() const
    //{
    //    return static_cast<int>(hubbard_orbitals_.size());
    //}

    //inline hubbard_orbital_descriptor_ const &hubbard_orbital(const int channel_) const
    //{
    //    assert(hubbard_orbitals_.size() > 0);
    //    return hubbard_orbitals_[channel_];
    //}

    //inline const std::vector<hubbard_orbital_descriptor_> &hubbard_orbital() const
    //{
    //    return hubbard_orbitals_;
    //}

    //inline void print_info() const;

    //inline int id() const
    //{
    //    return id_;
    //}

    inline int zn() const
    {
        assert(zn_ > 0);
        return zn_;
    }

    inline int zn(int zn__)
    {
        zn_ = zn__;
        return zn_;
    }

    inline std::string const& symbol() const
    {
        return symbol_;
    }

    inline std::string const& name() const
    {
        return name_;
    }

    inline double mass() const
    {
        return mass_;
    }

    ///// Return muffin-tin radius.
    ///** This is the last point of the radial grid. */
    //inline double mt_radius() const
    //{
    //    return radial_grid_.last();
    //}

    ///// Return number of muffin-tin radial grid points.
    //inline int num_mt_points() const
    //{
    //    assert(radial_grid_.num_points() > 0);
    //    return radial_grid_.num_points();
    //}

    //inline Radial_grid<double> const& radial_grid() const
    //{
    //    assert(radial_grid_.num_points() > 0);
    //    return radial_grid_;
    //}

    inline Radial_grid<double> const& free_atom_radial_grid() const
    {
        return free_atom_radial_grid_;
    }

    //inline double radial_grid(int ir) const
    //{
    //    return radial_grid_[ir];
    //}

    inline double free_atom_radial_grid(int ir) const
    {
        return free_atom_radial_grid_[ir];
    }

    inline int num_atomic_levels() const
    {
        return static_cast<int>(atomic_levels_.size());
    }

    inline atomic_level_descriptor const& atomic_level(int idx) const
    {
        return atomic_levels_[idx];
    }

    inline double num_core_electrons() const
    {
        return num_core_electrons_;
    }

    inline double num_valence_electrons() const
    {
        return num_valence_electrons_;
    }

    ///// Get free atom density at i-th point of radial grid.
    //inline double free_atom_density(const int idx) const
    //{
    //    return free_atom_density_spline_(idx);
    //}

    ///// Get free atom density at point x.
    //inline double free_atom_density(double x) const
    //{
    //    return free_atom_density_spline_.at_point(x);
    //}

    ///// Set the free atom all-electron density.
    //inline void free_atom_density(std::vector<double> rho__)
    //{
    //    free_atom_density_ = rho__;
    //}

    //inline int num_aw_descriptors() const
    //{
    //    return static_cast<int>(aw_descriptors_.size());
    //}

    //inline radial_solution_descriptor_set const& aw_descriptor(int l) const
    //{
    //    assert(l < (int)aw_descriptors_.size());
    //    return aw_descriptors_[l];
    //}

    //inline int num_lo_descriptors() const
    //{
    //    return (int)lo_descriptors_.size();
    //}

    //inline local_orbital_descriptor const& lo_descriptor(int idx) const
    //{
    //    return lo_descriptors_[idx];
    //}

    //inline int max_aw_order() const
    //{
    //    return max_aw_order_;
    //}

    ///// Order of augmented wave radial functions for a given l.
    //inline int aw_order(int l__) const
    //{
    //    return static_cast<int>(aw_descriptor(l__).size());
    //}

    //inline radial_functions_index const& indexr() const
    //{
    //    return indexr_;
    //}

    //inline radial_function_index_descriptor const& indexr(int i) const
    //{
    //    assert(i >= 0 && i < (int)indexr_.size());
    //    return indexr_[i];
    //}

    //inline int indexr_by_l_order(int l, int order) const
    //{
    //    return indexr_.index_by_l_order(l, order);
    //}

    //inline int indexr_by_idxlo(int idxlo) const
    //{
    //    return indexr_.index_by_idxlo(idxlo);
    //}

    //inline basis_functions_index const& indexb() const
    //{
    //    return indexb_;
    //}

    //inline basis_function_index_descriptor const& indexb(int i) const
    //{
    //    assert(i >= 0 && i < (int)indexb_.size());
    //    return indexb_[i];
    //}

    //inline int indexb_by_l_m_order(int l, int m, int order) const
    //{
    //    return indexb_.index_by_l_m_order(l, m, order);
    //}

    //inline int indexb_by_lm_order(int lm, int order) const
    //{
    //    return indexb_.index_by_lm_order(lm, order);
    //}

    //inline int mt_aw_basis_size() const
    //{
    //    return indexb_.size_aw();
    //}

    //inline int mt_lo_basis_size() const
    //{
    //    return indexb_.size_lo();
    //}

    //inline int mt_basis_size() const
    //{
    //    return indexb_.size();
    //}

    ///// Total number of radial basis functions.
    //inline int mt_radial_basis_size() const
    //{
    //    return indexr_.size();
    //}

    //inline void set_symbol(const std::string symbol__)
    //{
    //    symbol_ = symbol__;
    //}

    //inline void set_zn(int zn__)
    //{
    //    zn_ = zn__;
    //}

    //inline void set_mass(double mass__)
    //{
    //    mass_ = mass__;
    //}

    //inline void set_configuration(int n, int l, int k, double occupancy, bool core)
    //{
    //    atomic_level_descriptor level;
    //    level.n         = n;
    //    level.l         = l;
    //    level.k         = k;
    //    level.occupancy = occupancy;
    //    level.core      = core;
    //    atomic_levels_.push_back(level);
    //}

    ///// Return number of atoms of a given type.
    //inline int num_atoms() const
    //{
    //    return static_cast<int>(atom_id_.size());
    //}

    //inline int atom_id(int idx) const
    //{
    //    return atom_id_[idx];
    //}

    ///// Add global index of atom to this atom type.
    //inline void add_atom_id(int atom_id__)
    //{
    //    atom_id_.push_back(atom_id__);
    //}

    //inline bool initialized() const
    //{
    //    return initialized_;
    //}

    //inline std::string const& label() const
    //{
    //    return label_;
    //}

    //inline std::string const& file_name() const
    //{
    //    return file_name_;
    //}

    //inline int offset_lo() const
    //{
    //    assert(offset_lo_ >= 0);
    //    return offset_lo_;
    //}

    //inline void d_mtrx_ion(matrix<double> const& d_mtrx_ion__)
    //{
    //    d_mtrx_ion_ = matrix<double>(num_beta_radial_functions(), num_beta_radial_functions(),
    //                                 memory_t::host, "Atom_type::d_mtrx_ion_");
    //    d_mtrx_ion__ >> d_mtrx_ion_;
    //}

    //inline mdarray<double, 2> const& d_mtrx_ion() const
    //{
    //    return d_mtrx_ion_;
    //}

    //inline bool is_paw() const
    //{
    //    return is_paw_;
    //}

    //inline bool is_paw(bool is_paw__)
    //{
    //    is_paw_ = is_paw__;
    //    return is_paw_;
    //}

    //double paw_core_energy() const
    //{
    //    return paw_core_energy_;
    //}

    //double paw_core_energy(double paw_core_energy__)
    //{
    //    paw_core_energy_ = paw_core_energy__;
    //    return paw_core_energy_;
    //}

    //inline mdarray<int, 2> const& idx_radial_integrals() const
    //{
    //    return idx_radial_integrals_;
    //}

    //inline mdarray<double, 3>& rf_coef() const
    //{
    //    return rf_coef_;
    //}

    //inline mdarray<double, 3>& vrf_coef() const
    //{
    //    return vrf_coef_;
    //}

    //inline Simulation_parameters const& parameters() const
    //{
    //    return parameters_;
    //}

    //inline void set_free_atom_radial_grid(int num_points__, double const* points__)
    //{
    //    if (num_points__ <= 0) {
    //        TERMINATE("wrong number of radial points");
    //    }
    //    free_atom_radial_grid_ = Radial_grid_ext<double>(num_points__, points__);
    //}

    //inline double_complex f_coefficients(int xi1, int xi2, int s1, int s2) const
    //{
    //    return f_coefficients_(xi1, xi2, s1, s2);
    //}

    //inline Spline<double> const& q_radial_function(int idxrf1__, int idxrf2__, int l__) const
    //{
    //    if (idxrf1__ > idxrf2__) {
    //        std::swap(idxrf1__, idxrf2__);
    //    }
    //    /* combined index */
    //    int ijv = idxrf2__ * (idxrf2__ + 1) / 2 + idxrf1__;

    //    return q_radial_functions_l_(ijv, l__);
    //}

    //inline bool spin_orbit_coupling() const
    //{
    //    return spin_orbit_coupling_;
    //}

    //inline bool spin_orbit_coupling(bool so__)
    //{
    //    spin_orbit_coupling_ = so__;
    //    return spin_orbit_coupling_;
    //}

    //bool const& hubbard_correction() const
    //{
    //    return hubbard_correction_;
    //}


    //inline void set_hubbard_correction()
    //{
    //    this->hubbard_correction_ = true;
    //}


    ///// compare the angular, total angular momentum and radial part of
    ///// the beta projectors, leaving the m index free. Only useful
    ///// when spin orbit coupling is included.
    //inline bool compare_index_beta_functions(const int xi, const int xj) const
    //{
    //    return ((indexb(xi).l == indexb(xj).l) && (indexb(xi).idxrf == indexb(xj).idxrf) &&
    //            (std::abs(indexb(xi).j - indexb(xj).j) < 1e-8));
    //}

  //private:
  //  void read_hubbard_input();
  //  void generate_f_coefficients(void);
  //  inline double ClebschGordan(const int l, const double j, const double m, const int spin);
  //  inline double_complex calculate_U_sigma_m(const int l, const double j, const int mj, const int m, const int sigma);
};

//inline void Atom_type::init(int offset_lo__)
//{
//    PROFILE("sirius::Atom_type::init");
//
//    /* check if the class instance was already initialized */
//    if (initialized_) {
//        TERMINATE("can't initialize twice");
//    }
//
//    offset_lo_ = offset_lo__;
//
//    /* read data from file if it exists */
//    read_input(file_name_);
//
//    /* check the nuclear charge */
//    if (zn_ == 0) {
//        TERMINATE("zero atom charge");
//    }
//
//    if (parameters_.full_potential()) {
//        /* add valence levels to the list of atom's levels */
//        for (auto& e : atomic_conf[zn_ - 1]) {
//            /* check if this level is already in the list */
//            bool in_list{false};
//            for (auto& c : atomic_levels_) {
//                if (c.n == e.n && c.l == e.l && c.k == e.k) {
//                    in_list = true;
//                    break;
//                }
//            }
//            if (!in_list) {
//                auto level = e;
//                level.core = false;
//                atomic_levels_.push_back(level);
//            }
//        }
//        /* get the number of core electrons */
//        for (auto& e : atomic_levels_) {
//            if (e.core) {
//                num_core_electrons_ += e.occupancy;
//            }
//        }
//
//        /* initialize aw descriptors if they were not set manually */
//        if (aw_descriptors_.size() == 0) {
//            init_aw_descriptors(parameters_.lmax_apw());
//        }
//
//        if (static_cast<int>(aw_descriptors_.size()) != (parameters_.lmax_apw() + 1)) {
//            TERMINATE("wrong size of augmented wave descriptors");
//        }
//
//        max_aw_order_ = 0;
//        for (int l = 0; l <= parameters_.lmax_apw(); l++) {
//            max_aw_order_ = std::max(max_aw_order_, (int)aw_descriptors_[l].size());
//        }
//
//        if (max_aw_order_ > 3) {
//            TERMINATE("maximum aw order > 3");
//        }
//    }
//
//    if (!parameters_.full_potential()) {
//        /* add beta projectors to a list of atom's local orbitals */
//        for (auto& e: beta_radial_functions_) {
//            /* think of |beta> functions as of local orbitals */
//            local_orbital_descriptor lod;
//            lod.l = std::abs(e.first);
//
//            /* for spin orbit coupling; we can always do that there is
//               no insidence on the reset when calculations exclude SO */
//            if (e.first < 0) {
//                lod.total_angular_momentum = lod.l - 0.5;
//            } else {
//                lod.total_angular_momentum = lod.l + 0.5;
//            }
//            lo_descriptors_.push_back(lod);
//        }
//    }
//
//    /* initialize index of radial functions */
//    indexr_.init(aw_descriptors_, lo_descriptors_);
//
//    /* initialize index of muffin-tin basis functions */
//    indexb_.init(indexr_);
//
//    if (!parameters_.full_potential()) {
//        assert(mt_radial_basis_size() == num_beta_radial_functions());
//        assert(lmax_beta() == indexr().lmax());
//    }
//
//    /* get number of valence electrons */
//    num_valence_electrons_ = zn_ - num_core_electrons_;
//
//    int lmmax_pot = utils::lmmax(parameters_.lmax_pot());
//
//    if (parameters_.full_potential()) {
//        auto l_by_lm = utils::l_by_lm(parameters_.lmax_pot());
//
//        /* index the non-zero radial integrals */
//        std::vector<std::pair<int, int>> non_zero_elements;
//
//        for (int lm = 0; lm < lmmax_pot; lm++) {
//            int l = l_by_lm[lm];
//
//            for (int i2 = 0; i2 < indexr().size(); i2++) {
//                int l2 = indexr(i2).l;
//                for (int i1 = 0; i1 <= i2; i1++) {
//                    int l1 = indexr(i1).l;
//                    if ((l + l1 + l2) % 2 == 0) {
//                        if (lm) {
//                            non_zero_elements.push_back(std::pair<int, int>(i2, lm + lmmax_pot * i1));
//                        }
//                        for (int j = 0; j < parameters_.num_mag_dims(); j++) {
//                            int offs = (j + 1) * lmmax_pot * indexr().size();
//                            non_zero_elements.push_back(std::pair<int, int>(i2, lm + lmmax_pot * i1 + offs));
//                        }
//                    }
//                }
//            }
//        }
//        idx_radial_integrals_ = mdarray<int, 2>(2, non_zero_elements.size());
//        for (size_t j = 0; j < non_zero_elements.size(); j++) {
//            idx_radial_integrals_(0, j) = non_zero_elements[j].first;
//            idx_radial_integrals_(1, j) = non_zero_elements[j].second;
//        }
//    }
//
//    if (parameters_.processing_unit() == GPU && parameters_.full_potential()) {
//        idx_radial_integrals_.allocate(memory_t::device);
//        idx_radial_integrals_.copy<memory_t::host, memory_t::device>();
//        rf_coef_  = mdarray<double, 3>(num_mt_points(), 4, indexr().size(), memory_t::host_pinned | memory_t::device,
//                                       "Atom_type::rf_coef_");
//        vrf_coef_ = mdarray<double, 3>(num_mt_points(), 4, lmmax_pot * indexr().size() * (parameters_.num_mag_dims() + 1),
//                                       memory_t::host_pinned | memory_t::device, "Atom_type::vrf_coef_");
//    }
//
//    if (this->spin_orbit_coupling()) {
//        this->generate_f_coefficients();
//    }
//
//    if (is_paw()) {
//        if (num_beta_radial_functions() != num_ps_paw_wf()) {
//            TERMINATE("wrong number of pseudo wave-functions for PAW");
//        }
//        if (num_beta_radial_functions() != num_ae_paw_wf()) {
//            TERMINATE("wrong number of all-electron wave-functions for PAW");
//        }
//        ae_paw_wfs_array_ = mdarray<double, 2>(num_mt_points(), num_beta_radial_functions());
//        ae_paw_wfs_array_.zero();
//        ps_paw_wfs_array_ = mdarray<double, 2>(num_mt_points(), num_beta_radial_functions());
//        ps_paw_wfs_array_.zero();
//
//        for (int i = 0; i < num_beta_radial_functions(); i++) {
//            std::copy(ae_paw_wf(i).begin(), ae_paw_wf(i).end(), &ae_paw_wfs_array_(0, i));
//            std::copy(ps_paw_wf(i).begin(), ps_paw_wf(i).end(), &ps_paw_wfs_array_(0, i));
//        }
//    }
//
//    initialized_ = true;
//}
//
//inline void Atom_type::init_free_atom_density(bool smooth)
//{
//    free_atom_density_spline_ = Spline<double>(free_atom_radial_grid_, free_atom_density_);
//
//    /* smooth free atom density inside the muffin-tin sphere */
//    if (smooth) {
//        /* find point on the grid close to the muffin-tin radius */
//        int irmt = free_atom_radial_grid_.index_of(mt_radius());
//        /* interpolate at this point near MT radius */
//        double R = free_atom_radial_grid_[irmt];
//
//        //std::vector<double> g;
//        //double nel = fourpi * free_atom_density_spline_.integrate(g, 2);
//
//        //std::cout << "number of electrons: " << nel << ", inside MT: " << g[irmt] * fourpi << "\n";
//
//
//        //mdarray<double, 1> b(2);
//        //mdarray<double, 2> A(2, 2);
//        //A(0, 0) = std::pow(R, 2);
//        //A(0, 1) = std::pow(R, 3);
//        //A(1, 0) = 2 * R;
//        //A(1, 1) = 3 * std::pow(R, 2);
//        ////A(0, 0) = 4; //std::pow(R, 1);
//        ////A(0, 1) = std::pow(R, 1);
//        ////A(1, 0) = 0;
//        ////A(1, 1) = 1; //2 * std::pow(R, 1);
//
//        //b(0) = free_atom_density_spline_(irmt);
//        //b(1) = free_atom_density_spline_.deriv(1, irmt);
//
//        //linalg<CPU>::gesv<double>(2, 1, A.at<CPU>(), 2, b.at<CPU>(), 2);
//
//        //////== /* write initial density */
//        //std::stringstream sstr;
//        //sstr << "free_density_" << id_ << ".dat";
//        //FILE* fout = fopen(sstr.str().c_str(), "w");
//
//        //for (int ir = 0; ir < free_atom_radial_grid().num_points(); ir++) {
//        //    fprintf(fout, "%18.12f %18.12f \n", free_atom_radial_grid(ir), free_atom_density_[ir]);
//        //}
//        //fclose(fout);
//
//        /* make smooth free atom density inside muffin-tin */
//        for (int i = 0; i <= irmt; i++) {
//            double x = free_atom_radial_grid(i);
//            //free_atom_density_spline_(i) = b(0) * std::pow(free_atom_radial_grid(i), 2) + b(1) * std::pow(free_atom_radial_grid(i), 3);
//            free_atom_density_spline_(i) = free_atom_density_[i] * 0.5 * (1 + std::erf((x / R - 0.5) * 10));
//        }
//
//        /* interpolate new smooth density */
//        free_atom_density_spline_.interpolate();
//
//        ///* write smoothed density */
//        //sstr.str("");
//        //sstr << "free_density_modified_" << id_ << ".dat";
//        //fout = fopen(sstr.str().c_str(), "w");
//
//        //for (int ir = 0; ir < free_atom_radial_grid().num_points(); ir++) {
//        //    fprintf(fout, "%18.12f %18.12f \n", free_atom_radial_grid(ir), free_atom_density(ir));
//        //}
//        //fclose(fout);
//    }
//}
//
//inline void Atom_type::print_info() const
//{
//    printf("\n");
//    printf("label          : %s\n", label().c_str());
//    for (int i = 0; i < 80; i++) {
//        printf("-");
//    }
//    printf("\n");
//    printf("symbol         : %s\n", symbol_.c_str());
//    printf("name           : %s\n", name_.c_str());
//    printf("zn             : %i\n", zn_);
//    printf("mass           : %f\n", mass_);
//    printf("mt_radius      : %f\n", mt_radius());
//    printf("num_mt_points  : %i\n", num_mt_points());
//    printf("grid_origin    : %f\n", radial_grid_.first());
//    printf("grid_name      : %s\n", radial_grid_.name().c_str());
//    printf("\n");
//    printf("number of core electrons    : %f\n", num_core_electrons_);
//    printf("number of valence electrons : %f\n", num_valence_electrons_);
//
//    if (parameters_.hubbard_correction() && this->hubbard_correction_) {
//        printf("Hubbard correction is included in the calculations");
//        printf("\n");
//        printf("angular momentum         : %i\n", hubbard_orbitals_[0].hubbard_l());
//        printf("principal quantum number : %i\n", hubbard_orbitals_[0].hubbard_n());
//        printf("occupancy                : %f\n", hubbard_orbitals_[0].hubbard_occupancy());
//    }
//
//    if (parameters_.full_potential()) {
//        printf("\n");
//        printf("atomic levels (n, l, k, occupancy, core)\n");
//        for (int i = 0; i < (int)atomic_levels_.size(); i++) {
//            printf("%i  %i  %i  %8.4f %i\n", atomic_levels_[i].n, atomic_levels_[i].l, atomic_levels_[i].k,
//                   atomic_levels_[i].occupancy, atomic_levels_[i].core);
//        }
//        printf("\n");
//        printf("local orbitals\n");
//        for (int j = 0; j < (int)lo_descriptors_.size(); j++) {
//            printf("[");
//            for (int order = 0; order < (int)lo_descriptors_[j].rsd_set.size(); order++) {
//                if (order)
//                    printf(", ");
//                printf("{l : %2i, n : %2i, enu : %f, dme : %i, auto : %i}", lo_descriptors_[j].rsd_set[order].l,
//                       lo_descriptors_[j].rsd_set[order].n, lo_descriptors_[j].rsd_set[order].enu,
//                       lo_descriptors_[j].rsd_set[order].dme, lo_descriptors_[j].rsd_set[order].auto_enu);
//            }
//            printf("]\n");
//        }
//
//        printf("\n");
//        printf("augmented wave basis\n");
//        for (int j = 0; j < (int)aw_descriptors_.size(); j++) {
//            printf("[");
//            for (int order = 0; order < (int)aw_descriptors_[j].size(); order++) {
//                if (order)
//                    printf(", ");
//                printf("{l : %2i, n : %2i, enu : %f, dme : %i, auto : %i}", aw_descriptors_[j][order].l,
//                       aw_descriptors_[j][order].n, aw_descriptors_[j][order].enu, aw_descriptors_[j][order].dme,
//                       aw_descriptors_[j][order].auto_enu);
//            }
//            printf("]\n");
//        }
//        printf("maximum order of aw : %i\n", max_aw_order_);
//    }
//
//    printf("\n");
//    printf("total number of radial functions : %i\n", indexr().size());
//    printf("lmax of radial functions         : %i\n", indexr().lmax());
//    printf("max. number of radial functions  : %i\n", indexr().max_num_rf());
//    printf("total number of basis functions  : %i\n", indexb().size());
//    printf("number of aw basis functions     : %i\n", indexb().size_aw());
//    printf("number of lo basis functions     : %i\n", indexb().size_lo());
//}
//
//inline void Atom_type::read_input_core(json const& parser)
//{
//    std::string core_str = parser["core"];
//    if (int size = (int)core_str.size()) {
//        if (size % 2) {
//            std::stringstream s;
//            s << "wrong core configuration string : " << core_str;
//            TERMINATE(s);
//        }
//        int j = 0;
//        while (j < size) {
//            char c1 = core_str[j++];
//            char c2 = core_str[j++];
//
//            int n = -1;
//            int l = -1;
//
//            std::istringstream iss(std::string(1, c1));
//            iss >> n;
//
//            if (n <= 0 || iss.fail()) {
//                std::stringstream s;
//                s << "wrong principal quantum number : " << std::string(1, c1);
//                TERMINATE(s);
//            }
//
//            switch (c2) {
//                case 's': {
//                    l = 0;
//                    break;
//                }
//                case 'p': {
//                    l = 1;
//                    break;
//                }
//                case 'd': {
//                    l = 2;
//                    break;
//                }
//                case 'f': {
//                    l = 3;
//                    break;
//                }
//                default: {
//                    std::stringstream s;
//                    s << "wrong angular momentum label : " << std::string(1, c2);
//                    TERMINATE(s);
//                }
//            }
//
//            for (auto& e: atomic_conf[zn_ - 1]) {
//                if (e.n == n && e.l == l) {
//                    auto level = e;
//                    level.core = true;
//                    atomic_levels_.push_back(level);
//                }
//            }
//        }
//    }
//}
//
//inline void Atom_type::read_input_aw(json const& parser)
//{
//    radial_solution_descriptor rsd;
//    radial_solution_descriptor_set rsd_set;
//
//    /* default augmented wave basis */
//    rsd.n = -1;
//    rsd.l = -1;
//    for (size_t order = 0; order < parser["valence"][0]["basis"].size(); order++) {
//        rsd.enu      = parser["valence"][0]["basis"][order]["enu"];
//        rsd.dme      = parser["valence"][0]["basis"][order]["dme"];
//        rsd.auto_enu = parser["valence"][0]["basis"][order]["auto"];
//        aw_default_l_.push_back(rsd);
//    }
//
//    for (size_t j = 1; j < parser["valence"].size(); j++) {
//        rsd.l = parser["valence"][j]["l"];
//        rsd.n = parser["valence"][j]["n"];
//        rsd_set.clear();
//        for (size_t order = 0; order < parser["valence"][j]["basis"].size(); order++) {
//            rsd.enu      = parser["valence"][j]["basis"][order]["enu"];
//            rsd.dme      = parser["valence"][j]["basis"][order]["dme"];
//            rsd.auto_enu = parser["valence"][j]["basis"][order]["auto"];
//            rsd_set.push_back(rsd);
//        }
//        aw_specific_l_.push_back(rsd_set);
//    }
//}
//
//inline void Atom_type::read_input_lo(json const& parser)
//{
//    radial_solution_descriptor rsd;
//    radial_solution_descriptor_set rsd_set;
//
//    if (!parser.count("lo")) {
//        return;
//    }
//
//    int l;
//    for (size_t j = 0; j < parser["lo"].size(); j++) {
//        l = parser["lo"][j]["l"];
//
//        local_orbital_descriptor lod;
//        lod.l = l;
//        rsd.l = l;
//        rsd_set.clear();
//        for (size_t order = 0; order < parser["lo"][j]["basis"].size(); order++) {
//            rsd.n        = parser["lo"][j]["basis"][order]["n"];
//            rsd.enu      = parser["lo"][j]["basis"][order]["enu"];
//            rsd.dme      = parser["lo"][j]["basis"][order]["dme"];
//            rsd.auto_enu = parser["lo"][j]["basis"][order]["auto"];
//            rsd_set.push_back(rsd);
//        }
//        lod.rsd_set = rsd_set;
//        lo_descriptors_.push_back(lod);
//    }
//}
//
//
//inline void Atom_type::read_pseudo_uspp(json const& parser)
//{
//    symbol_ = parser["pseudo_potential"]["header"]["element"];
//
//    double zp;
//    zp  = parser["pseudo_potential"]["header"]["z_valence"];
//    zn_ = int(zp + 1e-10);
//
//    int nmtp = parser["pseudo_potential"]["header"]["mesh_size"];
//
//    auto rgrid = parser["pseudo_potential"]["radial_grid"].get<std::vector<double>>();
//    if (static_cast<int>(rgrid.size()) != nmtp) {
//        TERMINATE("wrong mesh size");
//    }
//    /* set the radial grid */
//    set_radial_grid(nmtp, rgrid.data());
//
//    local_potential(parser["pseudo_potential"]["local_potential"].get<std::vector<double>>());
//
//    ps_core_charge_density(parser["pseudo_potential"].value("core_charge_density", std::vector<double>(rgrid.size(), 0)));
//
//    ps_total_charge_density(parser["pseudo_potential"]["total_charge_density"].get<std::vector<double>>());
//
//    if (local_potential().size() != rgrid.size() || ps_core_charge_density().size() != rgrid.size() ||
//        ps_total_charge_density().size() != rgrid.size()) {
//        std::cout << local_potential().size() << " " << ps_core_charge_density().size() << " "
//                  << ps_total_charge_density().size() << std::endl;
//        TERMINATE("wrong array size");
//    }
//
//    if (parser["pseudo_potential"]["header"].count("spin_orbit")) {
//        spin_orbit_coupling_ = parser["pseudo_potential"]["header"].value("spin_orbit", spin_orbit_coupling_);
//    }
//
//    int nbf = parser["pseudo_potential"]["header"]["number_of_proj"];
//
//    for (int i = 0; i < nbf; i++) {
//        auto beta = parser["pseudo_potential"]["beta_projectors"][i]["radial_function"].get<std::vector<double>>();
//        if (static_cast<int>(beta.size()) > num_mt_points()) {
//            std::stringstream s;
//            s << "wrong size of beta functions for atom type " << symbol_ << " (label: " << label_ << ")" << std::endl
//              << "size of beta radial functions in the file: " << beta.size() << std::endl
//              << "radial grid size: " << num_mt_points();
//            TERMINATE(s);
//        }
//        int l = parser["pseudo_potential"]["beta_projectors"][i]["angular_momentum"];
//        if (spin_orbit_coupling_) {
//            // we encode the fact that the total angular momentum j = l
//            // -1/2 or l + 1/2 by changing the sign of l
//
//            double j = parser["pseudo_potential"]["beta_projectors"][i]["total_angular_momentum"];
//            if (j < (double)l) {
//                l *= -1;
//            }
//        }
//        add_beta_radial_function(l, beta);
//    }
//
//    mdarray<double, 2> d_mtrx(nbf, nbf);
//    d_mtrx.zero();
//    auto v = parser["pseudo_potential"]["D_ion"].get<std::vector<double>>();
//
//    for (int i = 0; i < nbf; i++) {
//        for (int j = 0; j < nbf; j++) {
//            d_mtrx(i, j) = v[j * nbf + i];
//        }
//    }
//    d_mtrx_ion(d_mtrx);
//
//    if (parser["pseudo_potential"].count("augmentation")) {
//        for (size_t k = 0; k < parser["pseudo_potential"]["augmentation"].size(); k++) {
//            int i    = parser["pseudo_potential"]["augmentation"][k]["i"];
//            int j    = parser["pseudo_potential"]["augmentation"][k]["j"];
//            //int idx  = j * (j + 1) / 2 + i;
//            int l    = parser["pseudo_potential"]["augmentation"][k]["angular_momentum"];
//            auto qij = parser["pseudo_potential"]["augmentation"][k]["radial_function"].get<std::vector<double>>();
//            if ((int)qij.size() != num_mt_points()) {
//                TERMINATE("wrong size of qij");
//            }
//            add_q_radial_function(i, j, l, qij);
//        }
//    }
//
//    /* read starting wave functions ( UPF CHI ) */
//    if (parser["pseudo_potential"].count("atomic_wave_functions")) {
//        size_t nwf = parser["pseudo_potential"]["atomic_wave_functions"].size();
//        std::vector<double> occupancies;
//        for (size_t k = 0; k < nwf; k++) {
//            //std::pair<int, std::vector<double>> wf;
//            auto v = parser["pseudo_potential"]["atomic_wave_functions"][k]["radial_function"].get<std::vector<double>>();
//
//            if ((int)v.size() != num_mt_points()) {
//                std::stringstream s;
//                s << "wrong size of atomic functions for atom type " << symbol_ << " (label: " << label_ << ")"
//                  << std::endl
//                  << "size of atomic radial functions in the file: " << v.size() << std::endl
//                  << "radial grid size: " << num_mt_points();
//                TERMINATE(s);
//            }
//
//            int l = parser["pseudo_potential"]["atomic_wave_functions"][k]["angular_momentum"];
//            int n = -1;
//            if (parser["pseudo_potential"]["atomic_wave_functions"][k].count("occupation")) {
//                occupancies.push_back(parser["pseudo_potential"]["atomic_wave_functions"][k]["occupation"]);
//            }
//
//            if (parser["pseudo_potential"]["atomic_wave_functions"][k].count("label")) {
//                std::string c1 = parser["pseudo_potential"]["atomic_wave_functions"][k]["label"];
//                std::istringstream iss(std::string(1, c1[0]));
//                iss >> n;
//            }
//
//            if (spin_orbit_coupling() &&
//                parser["pseudo_potential"]["atomic_wave_functions"][k].count("total_angular_momentum")) {
//                // check if j = l +- 1/2
//                if (parser["pseudo_potential"]["atomic_wave_functions"][k]["total_angular_momentum"] < l) {
//                    l = -l;
//                }
//            }
//            add_ps_atomic_wf(n, l, v);
//        }
//        ps_atomic_wf_occ(occupancies);
//    }
//}
//
//inline void Atom_type::read_pseudo_paw(json const& parser)
//{
//    is_paw_ = true;
//
//    /* read core energy */
//    paw_core_energy(parser["pseudo_potential"]["header"]["paw_core_energy"]);
//
//    /* cutoff index */
//    int cutoff_radius_index = parser["pseudo_potential"]["header"]["cutoff_radius_index"];
//
//    /* read core density and potential */
//    paw_ae_core_charge_density(parser["pseudo_potential"]["paw_data"]["ae_core_charge_density"].get<std::vector<double>>());
//
//    /* read occupations */
//    paw_wf_occ(parser["pseudo_potential"]["paw_data"]["occupations"].get<std::vector<double>>());
//
//    /* setups for reading AE and PS basis wave functions */
//    int num_wfc = num_beta_radial_functions();
//
//    /* read ae and ps wave functions */
//    for (int i = 0; i < num_wfc; i++) {
//        /* read ae wave func */
//        auto wfc = parser["pseudo_potential"]["paw_data"]["ae_wfc"][i]["radial_function"].get<std::vector<double>>();
//
//        if ((int)wfc.size() > num_mt_points()) {
//            std::stringstream s;
//            s << "wrong size of ae_wfc functions for atom type " << symbol_ << " (label: " << label_ << ")" << std::endl
//              << "size of ae_wfc radial functions in the file: " << wfc.size() << std::endl
//              << "radial grid size: " << num_mt_points();
//            TERMINATE(s);
//        }
//
//        add_ae_paw_wf(std::vector<double>(wfc.begin(), wfc.begin() + cutoff_radius_index));
//
//        wfc = parser["pseudo_potential"]["paw_data"]["ps_wfc"][i]["radial_function"].get<std::vector<double>>();
//
//        if ((int)wfc.size() > num_mt_points()) {
//            std::stringstream s;
//            s << "wrong size of ps_wfc functions for atom type " << symbol_ << " (label: " << label_ << ")" << std::endl
//              << "size of ps_wfc radial functions in the file: " << wfc.size() << std::endl
//              << "radial grid size: " << num_mt_points();
//            TERMINATE(s);
//        }
//
//        add_ps_paw_wf(std::vector<double>(wfc.begin(), wfc.begin() + cutoff_radius_index));
//    }
//}
//
//inline void Atom_type::read_input(std::string const& str__)
//{
//    json parser = utils::read_json_from_file_or_string(str__);
//
//    if (parser.empty()) {
//        return;
//    }
//
//    if (!parameters_.full_potential()) {
//        read_pseudo_uspp(parser);
//
//        if (parser["pseudo_potential"].count("paw_data")) {
//            read_pseudo_paw(parser);
//        }
//    }
//
//    if (parameters_.full_potential()) {
//        name_     = parser["name"];
//        symbol_   = parser["symbol"];
//        mass_     = parser["mass"];
//        zn_       = parser["number"];
//        double r0 = parser["rmin"];
//        double R  = parser["rmt"];
//        int nmtp  = parser["nrmt"];
//
//        auto rg = get_radial_grid_t(parameters_.settings().radial_grid_);
//
//        set_radial_grid(rg.first, nmtp, r0, R, rg.second);
//
//        read_input_core(parser);
//
//        read_input_aw(parser);
//
//        read_input_lo(parser);
//
//        /* create free atom radial grid */
//        auto fa_r              = parser["free_atom"]["radial_grid"].get<std::vector<double>>();
//        free_atom_radial_grid_ = Radial_grid_ext<double>(static_cast<int>(fa_r.size()), fa_r.data());
//        /* read density */
//        free_atom_density_ = parser["free_atom"]["density"].get<std::vector<double>>();
//    }
//
//    /* it is already done in input.h; here the different constans are initialized */
//    read_hubbard_input();
//}
//
//// TODO: this is not an atom type property, move to SHT or utils class
//inline double Atom_type::ClebschGordan(const int l, const double j, const double mj, const int spin)
//{
//    // l : orbital angular momentum
//    // m:  projection of the total angular momentum $m \pm /frac12$
//    // spin: Component of the spinor, 0 up, 1 down
//
//    double CG = 0.0; // Clebsch Gordan coeeficient cf PRB 71, 115106 page 3 first column
//
//    if ((spin != 0) && (spin != 1)) {
//        printf("Error : unkown spin direction\n");
//    }
//
//    const double denom = sqrt(1.0 / (2.0 * l + 1.0));
//
//    if (std::abs(j - l - 0.5) < 1e-8) { // check for j = l + 1/2
//        int m = static_cast<int>(mj - 0.5); // if mj is integer (2 * m), then int m = (mj-1) >> 1;
//        if (spin == 0) {
//            CG = sqrt(l + m + 1.0);
//        }
//        if (spin == 1) {
//            CG = sqrt((l - m));
//        }
//    } else {
//        if (std::abs(j - l + 0.5) < 1e-8) { // check for j = l - 1/2
//            int m = static_cast<int>(mj + 0.5);
//            if (m < (1 - l)) {
//                CG = 0.0;
//            } else {
//                if (spin == 0) {
//                    CG = sqrt(l - m + 1);
//                }
//                if (spin == 1) {
//                    CG = -sqrt(l + m);
//                }
//            }
//        } else {
//            printf("Clebsch gordan coefficients do not exist for this combination of j=%.5lf and l=%d\n", j, l);
//            exit(0);
//        }
//    }
//    return (CG * denom);
//}
//
//// this function computes the U^sigma_{ljm mj} coefficient that
//// rotates the complex spherical harmonics to the real one for the
//// spin orbit case
//
//// mj is normally half integer from -j to j but to avoid computation
//// error it is considered as integer so mj = 2 mj
//
//inline double_complex
//Atom_type::calculate_U_sigma_m(const int l, const double j, const int mj, const int mp, const int sigma)
//{
//
//    if ((sigma != 0) && (sigma != 1)) {
//        printf("SphericalIndex function : unkown spin direction\n");
//        return 0;
//    }
//
//    if (std::abs(j - l - 0.5) < 1e-8) {
//        // j = l + 1/2
//        // m = mj - 1/2
//
//        int m1 = (mj - 1) >> 1;
//        if (sigma == 0) { // up spin
//            if (m1 < -l) { // convention U^s_{mj,m'} = 0
//                return 0.0;
//            } else {// U^s_{mj,mp} =
//                return SHT::rlm_dot_ylm(l, m1, mp);
//            }
//        } else { // down spin
//            if ((m1 + 1) > l) {
//                return 0.0;
//            } else {
//                return SHT::rlm_dot_ylm(l, m1 + 1, mp);
//            }
//        }
//    } else {
//        if (std::abs(j - l + 0.5) < 1e-8) {
//            int m1 = (mj + 1) >> 1;
//            if (sigma == 0) {
//                return SHT::rlm_dot_ylm(l, m1 - 1, mp);
//            } else {
//                return SHT::rlm_dot_ylm(l, m1, mp);
//            }
//        } else {
//            printf("Spherical Index function : l and j are not compatible\n");
//            exit(0);
//        }
//    }
//}
//
//inline void Atom_type::generate_f_coefficients(void)
//{
//    // we consider Pseudo potentials with spin orbit couplings
//
//    // First thing, we need to compute the
//    // \f[f^{\sigma\sigma^\prime}_{l,j,m;l\prime,j\prime,m\prime}\f]
//    // They are defined by Eq.9 of Ref PRB 71, 115106
//    // and correspond to transformations of the
//    // spherical harmonics
//    if (!this->spin_orbit_coupling()) {
//        return;
//    }
//
//    // number of beta projectors
//    int nbf         = this->mt_basis_size();
//    f_coefficients_ = mdarray<double_complex, 4>(nbf, nbf, 2, 2);
//    f_coefficients_.zero();
//
//    for (int xi2 = 0; xi2 < nbf; xi2++) {
//        const int l2    = this->indexb(xi2).l;
//        const double j2 = this->indexb(xi2).j;
//        const int m2    = this->indexb(xi2).m;
//        for (int xi1 = 0; xi1 < nbf; xi1++) {
//            const int l1    = this->indexb(xi1).l;
//            const double j1 = this->indexb(xi1).j;
//            const int m1    = this->indexb(xi1).m;
//
//            if ((l2 == l1) && (std::abs(j1 - j2) < 1e-8)) {
//                // take beta projectors with same l and j
//                for (auto sigma2 = 0; sigma2 < 2; sigma2++) {
//                    for (auto sigma1 = 0; sigma1 < 2; sigma1++) {
//                        double_complex coef = {0.0, 0.0};
//
//                        // yes durty but loop over double is worst.
//                        // since mj is only important for the rotation
//                        // of the spherical harmonics the code takes
//                        // into account this odd convention.
//
//                        int jj1 = static_cast<int>(2.0 * j1 + 1e-8);
//                        for (int mj = -jj1; mj <= jj1; mj += 2) {
//                            coef += calculate_U_sigma_m(l1, j1, mj, m1, sigma1) *
//                                    this->ClebschGordan(l1, j1, mj / 2.0, sigma1) *
//                                    std::conj(calculate_U_sigma_m(l2, j2, mj, m2, sigma2)) *
//                                    this->ClebschGordan(l2, j2, mj / 2.0, sigma2);
//                        }
//                        f_coefficients_(xi1, xi2, sigma1, sigma2) = coef;
//                    }
//                }
//            }
//        }
//    }
//}
//
//inline void Atom_type::read_hubbard_input()
//{
//    if(!parameters_.Hubbard().hubbard_correction_) {
//        return;
//    }
//
//    this->hubbard_correction_ = false;
//
//    for(auto &d: parameters_.Hubbard().species) {
//        if (d.first == symbol_) {
//            int hubbard_l_ = d.second.l;
//            int hubbard_n_ = d.second.n;
//            if (hubbard_l_ < 0) {
//                std::istringstream iss(std::string(1, d.second.level[0]));
//                iss >> hubbard_n_;
//
//                if (hubbard_n_ <= 0 || iss.fail()) {
//                    std::stringstream s;
//                    s << "wrong principal quantum number : " << std::string(1, d.second.level[0]);
//                    TERMINATE(s);
//                }
//
//                switch (d.second.level[1]) {
//                case 's': {
//                    hubbard_l_ = 0;
//                    break;
//                }
//                case 'p': {
//                    hubbard_l_ = 1;
//                    break;
//                }
//                case 'd': {
//                    hubbard_l_ = 2;
//                    break;
//                }
//                case 'f': {
//                    hubbard_l_ = 3;
//                    break;
//                }
//                default: {
//                    std::stringstream s;
//                    s << "wrong angular momentum label : " << std::string(1, d.second.level[1]);
//                    TERMINATE(s);
//                }
//                }
//            }
//
//            add_hubbard_orbital(hubbard_n_,
//                                hubbard_l_,
//                                d.second.occupancy_,
//                                d.second.coeff_[0],
//                                d.second.coeff_[1],
//                                &d.second.coeff_[0],
//                                d.second.coeff_[4],
//                                d.second.coeff_[5],
//                                0.0);
//
//            this->hubbard_correction_ = true;
//        }
//    }
//}
} // namespace

#endif // __ATOM_TYPE_BASE_HPP__

