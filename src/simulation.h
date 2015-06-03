// Copyright (c) 2013-2015 Anton Kozhevnikov, Thomas Schulthess
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

/** \file simulation.h
 *   
 *  \brief Contains definition and implementation of Simulation_parameters and Simulation_context classes.
 */

#ifndef __SIMULATION_H__
#define __SIMULATION_H__

#include "input.h"
#include "mpi_grid.h"
#include "step_function.h"

/// SIRIUS namespace.
namespace sirius {

/// Parameters of the simulation. 
/** Parameters are first initialized from the initial input parameters and then by set..() methods.
 *  Any parameter used in the simulation must be first initialized here. Then the instance of the 
 *  Simulation_context class can be created where proper values of some parameters are set.
 */
class Simulation_parameters
{
    private:
    
        /// Maximum l for APW functions.
        int lmax_apw_;
        
        /// Maximum l for plane waves.
        int lmax_pw_;
        
        /// Maximum l for density.
        int lmax_rho_;
        
        /// Maximum l for potential
        int lmax_pot_;

        /// Maximum l for beta-projectors of the pseudopotential method.
        int lmax_beta_;
    
        /// Cutoff for augmented-wave functions.
        double aw_cutoff_;
    
        /// Cutoff for plane-waves (for density and potential expansion).
        double pw_cutoff_;
    
        /// Cutoff for |G+k| plane-waves.
        double gk_cutoff_;
        
        /// number of first-variational states
        int num_fv_states_;
    
        /// number of bands (= number of spinor states)
        int num_bands_;
       
        /// number of spin componensts (1 or 2)
        int num_spins_;
    
        /// number of dimensions of the magnetization and effective magnetic field (0, 1 or 3)
        int num_mag_dims_;
    
        /// true if spin-orbit correction is applied
        bool so_correction_;
       
        /// true if UJ correction is applied
        bool uj_correction_;
    
        /// MPI grid dimensions
        std::vector<int> mpi_grid_dims_;
        
        /// Starting time of the program.
        timeval start_time_;
    
        ev_solver_t std_evp_solver_type_;
    
        ev_solver_t gen_evp_solver_type_;
    
        /// Type of the processing unit.
        processing_unit_t processing_unit_;
    
        /// Smearing function width.
        double smearing_width_;

        int num_fft_threads_;

        int num_fft_workers_;

        int cyclic_block_size_;

        electronic_structure_method_t esm_type_;

        Iterative_solver_input_section iterative_solver_input_section_;
        
        XC_functionals_input_section xc_functionals_input_section_;
        
        Mixer_input_section mixer_input_section_;

        Unit_cell_input_section unit_cell_input_section_;

        std::map<std::string, ev_solver_t> str_to_ev_solver_t_;
        
        /// Import data from initial input parameters.
        void import(Input_parameters const& iip__)
        {
            mpi_grid_dims_  = iip__.common_input_section_.mpi_grid_dims_;
            num_fv_states_  = iip__.common_input_section_.num_fv_states_;
            smearing_width_ = iip__.common_input_section_.smearing_width_;
            
            std::string evsn[] = {iip__.common_input_section_.std_evp_solver_type_, iip__.common_input_section_.gen_evp_solver_type_};
            ev_solver_t* evst[] = {&std_evp_solver_type_, &gen_evp_solver_type_};

            for (int i = 0; i < 2; i++)
            {
                auto name = evsn[i];

                if (str_to_ev_solver_t_.count(name) == 0) TERMINATE("wrong eigen value solver");
                *evst[i] = str_to_ev_solver_t_[name];
            }

            std::string pu = iip__.common_input_section_.processing_unit_;
            std::transform(pu.begin(), pu.end(), pu.begin(), ::tolower);

            if (pu == "cpu")
            {
                processing_unit_ = CPU;
            }
            else if (pu == "gpu")
            {
                processing_unit_ = GPU;
            }
            else
            {
                TERMINATE("wrong processing unit");
            }

            std::string esm = iip__.common_input_section_.electronic_structure_method_;
            std::transform(esm.begin(), esm.end(), esm.begin(), ::tolower);
            if (esm == "full_potential_lapwlo")
            {
                esm_type_ = full_potential_lapwlo;
            }
            else if (esm == "full_potential_pwlo")
            {
                esm_type_ = full_potential_pwlo;
            }
            else if (esm == "ultrasoft_pseudopotential")
            {
                esm_type_ = ultrasoft_pseudopotential;
            } 
            else if (esm == "norm_conserving_pseudopotential")
            {
                esm_type_ = norm_conserving_pseudopotential;
            }
            else
            {
                TERMINATE("wrong type of electronic structure method");
            }

            iterative_solver_input_section_ = iip__.iterative_solver_input_section();
            xc_functionals_input_section_   = iip__.xc_functionals_input_section();
            mixer_input_section_            = iip__.mixer_input_section();
            unit_cell_input_section_        = iip__.unit_cell_input_section();

            cyclic_block_size_              = iip__.common_input_section_.cyclic_block_size_;

            num_fft_threads_                = iip__.common_input_section_.num_fft_threads_;
            num_fft_workers_                = iip__.common_input_section_.num_fft_workers_;
        }
    
    public:

        /// Create and initialize simulation parameters.
        /** The order of initialization is the following:
         *    - first, the default parameter values are set in the constructor
         *    - second, import() method is called and the parameters are overwritten with the input parameters
         *    - third, the user sets the values with set...() metods
         *    - fourh, the Simulation_context creates the copy of parameters and chekcs/sets the correct values
         */
        Simulation_parameters(Input_parameters const& iip__)
            : lmax_apw_(8), 
              lmax_pw_(-1), 
              lmax_rho_(8), 
              lmax_pot_(8),
              lmax_beta_(-1),
              aw_cutoff_(7.0), 
              pw_cutoff_(20.0), 
              gk_cutoff_(5.0), 
              num_fv_states_(-1), 
              num_spins_(1), 
              num_mag_dims_(0), 
              so_correction_(false), 
              uj_correction_(false),
              std_evp_solver_type_(ev_lapack),
              gen_evp_solver_type_(ev_lapack),
              processing_unit_(CPU),
              smearing_width_(0.001), 
              cyclic_block_size_(32),
              esm_type_(full_potential_lapwlo)
        {
            /* get the starting time */
            //gettimeofday(&start_time_, NULL);

            str_to_ev_solver_t_["lapack"]    = ev_lapack;
            str_to_ev_solver_t_["scalapack"] = ev_scalapack;
            str_to_ev_solver_t_["elpa1"]     = ev_elpa1;
            str_to_ev_solver_t_["elpa2"]     = ev_elpa2;
            str_to_ev_solver_t_["magma"]     = ev_magma;
            str_to_ev_solver_t_["plasma"]    = ev_plasma;
            str_to_ev_solver_t_["rs_cpu"]    = ev_rs_cpu;
            str_to_ev_solver_t_["rs_gpu"]    = ev_rs_gpu;

            import(iip__);
        }
            
        ~Simulation_parameters()
        {
        }
    
        inline void set_lmax_apw(int lmax_apw__)
        {
            lmax_apw_ = lmax_apw__;
        }
    
        inline void set_lmax_rho(int lmax_rho__)
        {
            lmax_rho_ = lmax_rho__;
        }
    
        inline void set_lmax_pot(int lmax_pot__)
        {
            lmax_pot_ = lmax_pot__;
        }

        inline void set_lmax_pw(int lmax_pw__)
        {
            lmax_pw_ = lmax_pw__;
        }

        inline void set_lmax_beta(int lmax_beta__)
        {
            lmax_beta_ = lmax_beta__;
        }
    
        void set_num_spins(int num_spins__)
        {
            num_spins_ = num_spins__;
        }
    
        void set_num_mag_dims(int num_mag_dims__)
        {
            num_mag_dims_ = num_mag_dims__;
        }

        inline void set_aw_cutoff(double aw_cutoff__)
        {
            aw_cutoff_ = aw_cutoff__;
        }

        /// Set plane-wave cutoff.
        inline void set_pw_cutoff(double pw_cutoff__)
        {
            pw_cutoff_ = pw_cutoff__;
        }
    
        inline void set_gk_cutoff(double gk_cutoff__)
        {
            gk_cutoff_ = gk_cutoff__;
        }
    
        inline void set_num_fv_states(int num_fv_states__)
        {
            num_fv_states_ = num_fv_states__;
        }

        inline void set_so_correction(bool so_correction__)
        {
            so_correction_ = so_correction__; 
        }
    
        inline void set_uj_correction(bool uj_correction__)
        {
            uj_correction_ = uj_correction__; 
        }

        inline void set_num_bands(int num_bands__)
        {
            num_bands_ = num_bands__;
        }
    
        inline int lmax_apw() const
        {
            return lmax_apw_;
        }
    
        inline int lmmax_apw() const
        {
            return Utils::lmmax(lmax_apw_);
        }
        
        inline int lmax_pw() const
        {
            return lmax_pw_;
        }
    
        inline int lmmax_pw() const
        {
            return Utils::lmmax(lmax_pw_);
        }
        
        inline int lmax_rho() const
        {
            return lmax_rho_;
        }
    
        inline int lmmax_rho() const
        {
            return Utils::lmmax(lmax_rho_);
        }
        
        inline int lmax_pot() const
        {
            return lmax_pot_;
        }
    
        inline int lmmax_pot() const
        {
            return Utils::lmmax(lmax_pot_);
        }

        inline int lmax_beta() const
        {
            return lmax_beta_;
        }
    
        inline double aw_cutoff() const
        {
            return aw_cutoff_;
        }
    
        /// Return plane-wave cutoff for G-vectors.
        inline double pw_cutoff() const
        {
            return pw_cutoff_;
        }
    
        inline double gk_cutoff() const
        {
            return gk_cutoff_;
        }
    
        inline int num_fv_states() const
        {
            return num_fv_states_;
        }
    
        inline int num_bands() const
        {
            return num_bands_;
        }
        
        inline int num_spins() const
        {
            assert(num_spins_ == 1 || num_spins_ == 2);
            
            return num_spins_;
        }
    
        inline int num_mag_dims() const
        {
            assert(num_mag_dims_ == 0 || num_mag_dims_ == 1 || num_mag_dims_ == 3);
            
            return num_mag_dims_;
        }
    
        inline int max_occupancy() const
        {
            return (2 / num_spins());
        }
        
        inline bool so_correction() const
        {
            return so_correction_;
        }
        
        inline bool uj_correction() const
        {
            return uj_correction_;
        }
    
        inline processing_unit_t processing_unit() const
        {
            return processing_unit_;
        }
    
        inline double smearing_width() const
        {
            return smearing_width_;
        }
    
        bool need_sv() const
        {
            if (num_spins() == 2 || uj_correction() || so_correction()) return true;
            return false;
        }
        
        inline std::vector<int> const& mpi_grid_dims() const
        {
            return mpi_grid_dims_;
        }

        inline int num_fft_threads() const
        {
            return num_fft_threads_;
        }
    
        inline int num_fft_workers() const
        {
            return num_fft_workers_;
        }

        inline int cyclic_block_size() const
        {
            return cyclic_block_size_;
        }
    
        inline electronic_structure_method_t esm_type() const
        {
            return esm_type_;
        }
    
        inline wave_function_distribution_t wave_function_distribution() const
        {
            switch (esm_type_)
            {
                case full_potential_lapwlo:
                case full_potential_pwlo:
                {
                    return block_cyclic_2d;
                    break;
                }
                case ultrasoft_pseudopotential:
                case norm_conserving_pseudopotential:
                {
                    return slab;
                    break;
                }
                default:
                {
                    TERMINATE("wrong method type");
                }
            }
            return block_cyclic_2d;
        }
    
        inline ev_solver_t std_evp_solver_type() const
        {
            return std_evp_solver_type_;
        }
    
        inline ev_solver_t gen_evp_solver_type() const
        {
            return gen_evp_solver_type_;
        }

        inline Mixer_input_section const& mixer_input_section() const
        {
            return mixer_input_section_;
        }

        inline XC_functionals_input_section const& xc_functionals_input_section() const
        {
            return xc_functionals_input_section_;
        }

        inline Iterative_solver_input_section const& iterative_solver_input_section() const
        {
            return iterative_solver_input_section_;
        }

        Unit_cell_input_section const& unit_cell_input_section() const
        {
            return unit_cell_input_section_;
        }
};

class Simulation_context
{
    private:

        /// Parameters of simulcaiton.
        Simulation_parameters parameters_; 
    
        /// Communicator for this simulation.
        Communicator const& comm_;

        /// MPI grid for this simulation.
        MPI_grid mpi_grid_;
        
        /// Unit cell of the simulation.
        Unit_cell unit_cell_;

        /// Reciprocal lattice of the unit cell.
        Reciprocal_lattice* reciprocal_lattice_;

        /// Step function is used in full-potential methods.
        Step_function* step_function_;

        /// FFT wrapper for dense grid.
        FFT3D<CPU>* fft_;

        /// FFT wrapper for coarse grid.
        FFT3D<CPU>* fft_coarse_;

        #ifdef _GPU_
        FFT3D<GPU>* fft_gpu_;

        FFT3D<GPU>* fft_gpu_coarse_;
        #endif

        Real_space_prj* real_space_prj_;

        /// Creation time of the context.
        timeval start_time_;

        bool initialized_;

    public:
        
        Simulation_context(Simulation_parameters const& parameters__,
                           Communicator const& comm__)
            : parameters_(parameters__),
              comm_(comm__),
              unit_cell_(parameters_.esm_type(), comm_, parameters_.processing_unit()),
              reciprocal_lattice_(nullptr),
              step_function_(nullptr),
              fft_(nullptr),
              fft_coarse_(nullptr),
              #ifdef _GPU_
              fft_gpu_(nullptr),
              fft_gpu_coarse_(nullptr),
              #endif
              real_space_prj_(nullptr),
              initialized_(false)
        {
            gettimeofday(&start_time_, NULL);
            
            //==     tm* ptm = localtime(&start_time_.tv_sec); 
            //==     strftime(buf, sizeof(buf), fmt, ptm);
            //==     start_time("%Y%m%d%H%M%S")

            unit_cell_.import(parameters_.unit_cell_input_section());
        }

        ~Simulation_context()
        {
            if (fft_ != nullptr) delete fft_;
            if (fft_coarse_ != nullptr) delete fft_coarse_;
            #ifdef _GPU_
            if (fft_gpu_ != nullptr) delete fft_gpu_;
            if (fft_gpu_coarse_ != nullptr) delete fft_gpu_coarse_;
            #endif
            if (reciprocal_lattice_ != nullptr) delete reciprocal_lattice_;
            if (step_function_ != nullptr) delete step_function_;
            if (real_space_prj_ != nullptr) delete real_space_prj_;
        }

        inline bool full_potential()
        {
            return (parameters_.esm_type() == full_potential_lapwlo || parameters_.esm_type() == full_potential_pwlo);
        }

        /// Initialize the similation (can only be called once).
        void initialize()
        {
            if (initialized_) TERMINATE("Simulation context is already initialized.");

            switch (parameters_.esm_type())
            {
                case full_potential_lapwlo:
                {
                    break;
                }
                case full_potential_pwlo:
                {
                    parameters_.set_lmax_pw(parameters_.lmax_apw());
                    parameters_.set_lmax_apw(-1);
                    break;
                }
                case ultrasoft_pseudopotential:
                case norm_conserving_pseudopotential:
                {
                    parameters_.set_lmax_apw(-1);
                    parameters_.set_lmax_rho(-1);
                    parameters_.set_lmax_pot(-1);
                    break;
                }
            }

            /* check MPI grid dimensions and set a default grid if needed */
            auto mpi_grid_dims = parameters_.mpi_grid_dims();
            if (!mpi_grid_dims.size()) 
            {
                mpi_grid_dims = std::vector<int>(1);
                mpi_grid_dims[0] = comm_.size();
            }

            /* setup MPI grid */
            mpi_grid_ = MPI_grid(mpi_grid_dims, comm_);

            /* initialize variables, related to the unit cell */
            unit_cell_.initialize(parameters_.lmax_apw(), parameters_.lmax_pot(), parameters_.num_mag_dims());

            parameters_.set_lmax_beta(unit_cell_.lmax_beta());

            /* create FFT interface */
            fft_ = new FFT3D<CPU>(Utils::find_translation_limits(parameters_.pw_cutoff(), unit_cell_.reciprocal_lattice_vectors()),
                                  parameters_.num_fft_threads(), parameters_.num_fft_workers());
            
            fft_->init_gvec(parameters_.pw_cutoff(), unit_cell_.reciprocal_lattice_vectors());

            #ifdef _GPU_
            fft_gpu_ = new FFT3D<GPU>(fft_->grid_size(), 1);
            #endif
            
            if (parameters_.esm_type() == ultrasoft_pseudopotential ||
                parameters_.esm_type() == norm_conserving_pseudopotential)
            {
                /* create FFT interface for coarse grid */
                fft_coarse_ = new FFT3D<CPU>(Utils::find_translation_limits(parameters_.gk_cutoff() * 2, unit_cell_.reciprocal_lattice_vectors()),
                                             parameters_.num_fft_threads(), parameters_.num_fft_workers());
                
                fft_coarse_->init_gvec(parameters_.gk_cutoff() * 2, unit_cell_.reciprocal_lattice_vectors());

                #ifdef _GPU_
                fft_gpu_coarse_ = new FFT3D<GPU>(fft_coarse_->grid_size(), 2);
                #endif
            }
    
            if (unit_cell_.num_atoms() != 0) unit_cell_.symmetry()->check_gvec_symmetry(fft_);

            /* create a reciprocal lattice */
            int lmax = -1;
            switch (parameters_.esm_type())
            {
                case full_potential_lapwlo:
                {
                    lmax = parameters_.lmax_pot();
                    break;
                }
                case full_potential_pwlo:
                {
                    STOP();
                }
                case ultrasoft_pseudopotential:
                case norm_conserving_pseudopotential:
                {
                    lmax = 2 * parameters_.lmax_beta();
                    break;
                }
            }
            
            reciprocal_lattice_ = new Reciprocal_lattice(unit_cell_, parameters_.esm_type(), fft_, lmax, comm_);

            if (full_potential()) step_function_ = new Step_function(unit_cell_, reciprocal_lattice_, fft_, comm_);

            if (parameters_.iterative_solver_input_section().real_space_prj_) 
            {
                real_space_prj_ = new Real_space_prj(unit_cell_, comm_, parameters_.iterative_solver_input_section().R_mask_scale_,
                                                     parameters_.iterative_solver_input_section().mask_alpha_,
                                                     parameters_.gk_cutoff(), parameters_.num_fft_threads(),
                                                     parameters_.num_fft_workers());
            }

            /* take 20% of empty non-magnetic states */
            if (parameters_.num_fv_states() < 0) 
            {
                int nfv = int(1e-8 + unit_cell_.num_valence_electrons() / 2.0) +
                              std::max(10, int(0.1 * unit_cell_.num_valence_electrons()));
                parameters_.set_num_fv_states(nfv);
            }
            
            if (parameters_.num_fv_states() < int(unit_cell_.num_valence_electrons() / 2.0))
                TERMINATE("not enough first-variational states");
            
            /* total number of bands */
            parameters_.set_num_bands(parameters_.num_fv_states() * parameters_.num_spins());

            initialized_ = true;
        }

        Simulation_parameters const& parameters() const
        {
            return parameters_;
        }

        Unit_cell& unit_cell()
        {
            return unit_cell_;
        }

        Step_function const* step_function() const
        {
            return step_function_;
        }

        Reciprocal_lattice const* reciprocal_lattice() const
        {
            return reciprocal_lattice_;
        }

        inline FFT3D<CPU>* fft() const
        {
            return fft_;
        }

        inline FFT3D<CPU>* fft_coarse() const
        {
            return fft_coarse_;
        }

        #ifdef _GPU_
        inline FFT3D<GPU>* fft_gpu() const
        {
            return fft_gpu_;
        }

        inline FFT3D<GPU>* fft_gpu_coarse() const
        {
            return fft_gpu_coarse_;
        }
        #endif

        Communicator const& comm() const
        {
            return comm_;
        }

        MPI_grid const& mpi_grid() const
        {
            return mpi_grid_;
        }
        
        void create_storage_file() const
        {
            if (comm_.rank() == 0)
            {
                /* create new hdf5 file */
                HDF5_tree fout(storage_file_name, true);
                fout.create_node("parameters");
                fout.create_node("effective_potential");
                fout.create_node("effective_magnetic_field");
                fout.create_node("density");
                fout.create_node("magnetization");
                
                fout["parameters"].write("num_spins", parameters_.num_spins());
                fout["parameters"].write("num_mag_dims", parameters_.num_mag_dims());
                fout["parameters"].write("num_bands", parameters_.num_bands());
            }
            comm_.barrier();
        }

        Real_space_prj const* real_space_prj() const
        {
            return real_space_prj_;
        }

        void print_info()
        {
            printf("\n");
            printf("SIRIUS version : %2i.%02i\n", major_version, minor_version);
            printf("git hash       : %s\n", git_hash);
            printf("build date     : %s\n", build_date);
            //= printf("start time     : %s\n", start_time("%c").c_str());
            printf("\n");
            printf("number of MPI ranks           : %i\n", comm_.size());
            printf("MPI grid                      :");
            for (int i = 0; i < mpi_grid_.num_dimensions(); i++) printf(" %i", mpi_grid_.size(1 << i));
            printf("\n");
            printf("maximum number of OMP threads   : %i\n", Platform::max_num_threads()); 
            printf("number of OMP threads for FFT   : %i\n", parameters_.num_fft_threads()); 
            printf("number of pthreads for each FFT : %i\n", parameters_.num_fft_workers()); 
            printf("cyclic block size               : %i\n", parameters_.cyclic_block_size());
        
            unit_cell_.print_info();
        
            printf("\n");
            printf("plane wave cutoff                     : %f\n", parameters_.pw_cutoff());
            printf("number of G-vectors within the cutoff : %i\n", fft_->num_gvec());
            printf("number of G-shells                    : %i\n", fft_->num_gvec_shells_inner());
            printf("FFT grid size   : %i %i %i   total : %i\n", fft_->size(0), fft_->size(1), fft_->size(2), fft_->size());
            printf("FFT grid limits : %i %i   %i %i   %i %i\n", fft_->grid_limits(0).first, fft_->grid_limits(0).second,
                                                                fft_->grid_limits(1).first, fft_->grid_limits(1).second,
                                                                fft_->grid_limits(2).first, fft_->grid_limits(2).second);
            
            if (parameters_.esm_type() == ultrasoft_pseudopotential ||
                parameters_.esm_type() == norm_conserving_pseudopotential)
            {
                printf("number of G-vectors on the coarse grid within the cutoff : %i\n", fft_coarse_->num_gvec());
                printf("FFT coarse grid size   : %i %i %i   total : %i\n", fft_coarse_->size(0), fft_coarse_->size(1), fft_coarse_->size(2), fft_coarse_->size());
                printf("FFT coarse grid limits : %i %i   %i %i   %i %i\n", fft_coarse_->grid_limits(0).first, fft_coarse_->grid_limits(0).second,
                                                                           fft_coarse_->grid_limits(1).first, fft_coarse_->grid_limits(1).second,
                                                                           fft_coarse_->grid_limits(2).first, fft_coarse_->grid_limits(2).second);
            }
        
            for (int i = 0; i < unit_cell_.num_atom_types(); i++) unit_cell_.atom_type(i)->print_info();
        
            printf("\n");
            printf("total number of aw basis functions : %i\n", unit_cell_.mt_aw_basis_size());
            printf("total number of lo basis functions : %i\n", unit_cell_.mt_lo_basis_size());
            printf("number of first-variational states : %i\n", parameters_.num_fv_states());
            printf("number of bands                    : %i\n", parameters_.num_bands());
            printf("number of spins                    : %i\n", parameters_.num_spins());
            printf("number of magnetic dimensions      : %i\n", parameters_.num_mag_dims());
            printf("lmax_apw                           : %i\n", parameters_.lmax_apw());
            printf("lmax_pw                            : %i\n", parameters_.lmax_pw());
            printf("lmax_rho                           : %i\n", parameters_.lmax_rho());
            printf("lmax_pot                           : %i\n", parameters_.lmax_pot());
            printf("lmax_beta                          : %i\n", parameters_.lmax_beta());
        
            //== std::string evsn[] = {"standard eigen-value solver: ", "generalized eigen-value solver: "};
            //== ev_solver_t evst[] = {std_evp_solver_->type(), gen_evp_solver_->type()};
            //== for (int i = 0; i < 2; i++)
            //== {
            //==     printf("\n");
            //==     printf("%s", evsn[i].c_str());
            //==     switch (evst[i])
            //==     {
            //==         case ev_lapack:
            //==         {
            //==             printf("LAPACK\n");
            //==             break;
            //==         }
            //==         #ifdef _SCALAPACK_
            //==         case ev_scalapack:
            //==         {
            //==             printf("ScaLAPACK, block size %i\n", linalg<scalapack>::cyclic_block_size());
            //==             break;
            //==         }
            //==         case ev_elpa1:
            //==         {
            //==             printf("ELPA1, block size %i\n", linalg<scalapack>::cyclic_block_size());
            //==             break;
            //==         }
            //==         case ev_elpa2:
            //==         {
            //==             printf("ELPA2, block size %i\n", linalg<scalapack>::cyclic_block_size());
            //==             break;
            //==         }
            //==         case ev_rs_gpu:
            //==         {
            //==             printf("RS_gpu\n");
            //==             break;
            //==         }
            //==         case ev_rs_cpu:
            //==         {
            //==             printf("RS_cpu\n");
            //==             break;
            //==         }
            //==         #endif
            //==         case ev_magma:
            //==         {
            //==             printf("MAGMA\n");
            //==             break;
            //==         }
            //==         case ev_plasma:
            //==         {
            //==             printf("PLASMA\n");
            //==             break;
            //==         }
            //==         default:
            //==         {
            //==             error_local(__FILE__, __LINE__, "wrong eigen-value solver");
            //==         }
            //==     }
            //== }
        
            printf("\n");
            printf("processing unit : ");
            switch (parameters_.processing_unit())
            {
                case CPU:
                {
                    printf("CPU\n");
                    break;
                }
                case GPU:
                {
                    printf("GPU\n");
                    break;
                }
            }
            
            printf("\n");
            printf("XC functionals : \n");
            for (int i = 0; i < (int)parameters_.xc_functionals_input_section().xc_functional_names_.size(); i++)
            {
                std::string xc_label = parameters_.xc_functionals_input_section().xc_functional_names_[i];
                XC_functional xc(xc_label, parameters_.num_spins());
                printf("\n");
                printf("%s\n", xc_label.c_str());
                printf("%s\n", xc.name().c_str());
                printf("%s\n", xc.refs().c_str());
            }
        }
};

};

/** \page stdvarname Standard variable names
    
    Below is the list of standard names for some of the loop variables:
    
    l - index of orbital quantum number \n
    m - index of azimutal quantum nuber \n
    lm - combined index of (l,m) quantum numbers \n
    ia - index of atom \n
    ic - index of atom class \n
    iat - index of atom type \n
    ir - index of r-point \n
    ig - index of G-vector \n
    idxlo - index of local orbital \n
    idxrf - index of radial function \n

    The loc suffix is added to the variable to indicate that it runs over local fraction of elements for the given
    MPI rank. Typical code looks like this:
    
    \code{.cpp}
        // zero array
        memset(&mt_val[0], 0, parameters_.num_atoms() * sizeof(T));
        
        // loop over local fraction of atoms
        for (int ialoc = 0; ialoc < parameters_.spl_num_atoms().local_size(); ialoc++)
        {
            // get global index of atom
            int ia = parameters_.spl_num_atoms(ialoc);

            int nmtp = parameters_.atom(ia)->num_mt_points();
           
            // integrate spherical part of the function
            Spline<T> s(nmtp, parameters_.atom(ia)->type()->radial_grid());
            for (int ir = 0; ir < nmtp; ir++) s[ir] = f_mt<local>(0, ir, ialoc);
            mt_val[ia] = s.interpolate().integrate(2) * fourpi * y00;
        }

        // simple array synchronization
        Platform::allreduce(&mt_val[0], parameters_.num_atoms());
    \endcode
*/

#endif // __SIMULATION_H__
