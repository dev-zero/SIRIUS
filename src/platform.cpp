// Copyright (c) 2013-2014 Anton Kozhevnikov, Thomas Schulthess
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

/** \file platform.cpp
 *   
 *  \brief Contains remaining implementation of Platform class.
 */

#include "platform.h"

int Platform::num_fft_threads_ = -1;

#ifdef _PLASMA_
extern "C" void plasma_init(int num_cores);
#endif

#ifdef _RS_GEN_EIG_
extern "C" void libsci_acc_init();
#endif

void Platform::initialize(bool call_mpi_init, bool call_cublas_init)
{
    //if (call_mpi_init) MPI_Init(NULL, NULL);
    int provided;
    if (call_mpi_init) 
    {
        MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &provided);
        if (provided != MPI_THREAD_MULTIPLE)
        {
            printf("MPI_Init_thread didn't provide MPI_THREAD_MULTIPLE\n");
        }
    }

    #ifdef _GPU_
    //cuda_initialize();
    if (call_cublas_init) cublas_init();
    if (mpi_rank() == 0) cuda_device_info();
    cuda_create_streams(max_num_threads());
    #endif
    #ifdef _MAGMA_
    magma_init_wrapper();
    #endif
    #ifdef _PLASMA_
    plasma_init(max_num_threads());
    #endif
    #ifdef _RS_GEN_EIG_
    libsci_acc_init();
    #endif

    assert(sizeof(int) == 4);
    assert(sizeof(double) == 8);
}

void Platform::finalize()
{
    MPI_Finalize();
    #ifdef _MAGMA_
    magma_finalize_wrapper();
    #endif
    #ifdef _GPU_
    cuda_destroy_streams(max_num_threads());
    cuda_device_reset();
    #endif
}

int Platform::mpi_rank(MPI_Comm comm)
{
    int rank;
    MPI_Comm_rank(comm, &rank);
    return rank;
}

int Platform::num_mpi_ranks(MPI_Comm comm)
{
    int size;
    MPI_Comm_size(comm, &size);
    return size;
}

void Platform::abort()
{
    if (num_mpi_ranks() == 1)
    {
        raise(SIGTERM);
    }
    else
    {   
        MPI_Abort(MPI_COMM_WORLD, -13);
    }
    exit(-13);
}