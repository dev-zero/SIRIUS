#ifndef __SIRIUS_H__
#define __SIRIUS_H__

#include <assert.h>
#include <stdio.h>
#include <sys/time.h>

#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cmath>

#ifdef _FFTW_
#include <fftw3.h>
#endif 
extern "C" {
#include <spglib.h>
}
#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_complex.h>
#include <gsl/gsl_heapsort.h>
#include "../libjson/libjson.h"
     
#include "error_handling.h"
#include "timer.h"
#include "config.h"
#include "constants.h"
#include "mdarray.h"
#include "linalg.h"
#include "global_inl.h"
#include "radial_grid.h"
#include "spline.h"
#include "radial_solver.h"
#include "json_tree.h"
#include "atom_type.h"
#include "atom_symmetry_class.h"
#include "atom.h"
#include "fft3d.h"
#include "sirius_unit_cell.h"
#include "sirius_geometry.h"
#include "sirius_gvec.h"
#include "sirius_step_func.h"
#include "sirius_global.h"

sirius::SiriusGlobal sirius_global;

#endif // __SIRIUS_H__
