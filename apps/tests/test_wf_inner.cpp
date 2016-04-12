#include <sirius.h>


void test_reduce(int M, int N, int K, std::vector<int> mpi_grid)
{
    int bs = 32;
    BLACS_grid blacs_grid(mpi_comm_world(), mpi_grid[0], mpi_grid[1]);

    dmatrix<double_complex> c(M, N, blacs_grid, bs, bs);
    c.zero();

    mdarray<double_complex, 3> c_tmp(c.num_rows_local(0), c.num_cols_local(0), 2);
    c_tmp.zero();

    runtime::Timer t2("reduce");
    for (int rank_col = 0; rank_col < mpi_grid[1]; rank_col++)
    {
        for (int rank_row = 0; rank_row < mpi_grid[0]; rank_row++)
        {
            //mpi_comm_world().reduce(c_tmp.at<CPU>(0, 0, 0), c_tmp.ld() * c.num_cols_local(rank_col), blacs_grid.cart_rank(rank_row, rank_col));
           
            blacs_grid.comm_col().reduce(c_tmp.at<CPU>(0, 0, 0), c_tmp.ld() * c.num_cols_local(rank_col), rank_col);
            if (blacs_grid.rank_col() == rank_col)
                blacs_grid.comm_row().reduce(c_tmp.at<CPU>(0, 0, 0), c_tmp.ld() * c.num_cols_local(rank_col), rank_row);
        }
    }
    double tval2 = t2.stop();
    double perf2 = 8e-9 * M * N * K / tval2 / mpi_comm_world().size();
    if (mpi_comm_world().rank() == 0)
    {
        printf("reduction time (sec) : %12.6f\n", tval2);
        printf("absolute peak performance (GFlops / rank): %12.6f\n", perf2);
    }
}

void test_reduce_2(int M, int N, int K, std::vector<int> mpi_grid)
{
    int bs = 32;
    BLACS_grid blacs_grid(mpi_comm_world(), mpi_grid[0], mpi_grid[1]);

    dmatrix<double_complex> c(M, N, blacs_grid, bs, bs);
    c.zero();

    mdarray<double_complex, 3> c_tmp(c.num_rows_local(0), c.num_cols_local(0), 2);
    c_tmp.zero();

    runtime::Timer t2("reduce");

    std::array<MPI_Request, 2> req = {MPI_REQUEST_NULL, MPI_REQUEST_NULL};
    std::array<std::pair<int, int>, 2> pos;
    
    int s = 0;
    for (int rank_col = 0; rank_col < mpi_grid[1]; rank_col++)
    {
        for (int rank_row = 0; rank_row < mpi_grid[0]; rank_row++)
        {
            if (req[s % 2] != MPI_REQUEST_NULL)
            {
                MPI_Wait(&req[s % 2], MPI_STATUS_IGNORE);
            }

            pos[s % 2].first = rank_row;
            pos[s % 2].second = rank_col;
            
            mpi_comm_world().ireduce(c_tmp.at<CPU>(0, 0, s % 2), c_tmp.ld() * c.num_cols_local(rank_col), blacs_grid.cart_rank(rank_row, rank_col), &req[s % 2]);
            
            s++;
        }
    }

    for (int s: {0, 1})
    {
        if (req[s] != MPI_REQUEST_NULL)
        {
            MPI_Wait(&req[s], MPI_STATUS_IGNORE);
        }
    }


    double tval2 = t2.stop();
    double perf2 = 8e-9 * M * N * K / tval2 / mpi_comm_world().size();
    if (mpi_comm_world().rank() == 0)
    {
        printf("reduction time (sec) : %12.6f\n", tval2);
        printf("absolute peak performance (GFlops / rank): %12.6f\n", perf2);
    }
}

double test_gemm(int M, int N, int K, std::vector<int> mpi_grid)
{
    runtime::Timer t("test_gemm"); 

    int bs = 32;
    BLACS_grid blacs_grid(mpi_comm_world(), mpi_grid[0], mpi_grid[1]);

    splindex<block> spl_K(K, mpi_comm_world().size(), mpi_comm_world().rank());
    
    matrix<double_complex> a(spl_K.local_size(), M);
    matrix<double_complex> b(spl_K.local_size(), N);

    //matrix<double_complex> c(M, N);
    dmatrix<double_complex> c(M, N, blacs_grid, bs, bs);
    c.zero();

    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j < spl_K.local_size(); j++)
            a(j, i) = 0.1; //type_wrapper<double_complex>::random();
    }
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < spl_K.local_size(); j++)
            b(j, i) = 0.1; //type_wrapper<double_complex>::random();
    }
    mdarray<double_complex, 3> c_tmp(c.num_rows_local(0), c.num_cols_local(0), 2);
    c_tmp.zero();

    runtime::Timer t2("reduce_only");
    for (int rank_col = 0; rank_col < mpi_grid[1]; rank_col++)
    {
        for (int rank_row = 0; rank_row < mpi_grid[0]; rank_row++)
        {
            mpi_comm_world().reduce(c_tmp.at<CPU>(0, 0, 0), c_tmp.ld() * c.num_cols_local(rank_col), blacs_grid.cart_rank(rank_row, rank_col));
        }
    }
    double tval2 = t2.stop();
    double perf2 = 8e-9 * M * N * K / tval2 / mpi_comm_world().size();
    if (mpi_comm_world().rank() == 0)
    {
        printf("reduction time (sec) : %12.6f\n", tval2);
        printf("absolute peak performance (GFlops / rank): %12.6f\n", perf2);
    }



    runtime::Timer t1("gemm_only");

    mdarray<double_complex, 2> a_tmp(spl_K.local_size(), c.num_rows_local(0));
    mdarray<double_complex, 2> b_tmp(spl_K.local_size(), c.num_cols_local(0));

    std::array<MPI_Request, 2> req = {MPI_REQUEST_NULL, MPI_REQUEST_NULL};
    std::array<std::pair<int, int>, 2> pos;
    
    int s = 0;
    for (int rank_col = 0; rank_col < mpi_grid[1]; rank_col++)
    {
        for (int rank_row = 0; rank_row < mpi_grid[0]; rank_row++)
        {
            #pragma omp parallel for
            for (int i = 0; i < c.num_rows_local(rank_row); i++)
                std::memcpy(&a_tmp(0, i), &a(0, c.spl_row().global_index(i, rank_row)), spl_K.local_size() * sizeof(double_complex));

            #pragma omp parallel for
            for (int i = 0; i < c.num_cols_local(rank_col); i++)
                std::memcpy(&b_tmp(0, i), &b(0, c.spl_col().global_index(i, rank_col)), spl_K.local_size() * sizeof(double_complex));

            if (req[s % 2] != MPI_REQUEST_NULL)
            {
                MPI_Wait(&req[s % 2], MPI_STATUS_IGNORE);

                if (mpi_comm_world().rank() == blacs_grid.cart_rank(pos[s % 2].first, pos[s % 2].second))
                {
                    #pragma omp parallel for
                    for (int i = 0; i < c.num_cols_local(); i++)
                        std::memcpy(&c(0, i), &c_tmp(0, i, s % 2), c.num_rows_local() * sizeof(double_complex));
                }
            }

            pos[s % 2].first = rank_row;
            pos[s % 2].second = rank_col;
            
            linalg<CPU>::gemm(2, 0, c.num_rows_local(rank_row), c.num_cols_local(rank_col), spl_K.local_size(),
                              a_tmp.at<CPU>(), a_tmp.ld(), b_tmp.at<CPU>(), b_tmp.ld(), c_tmp.at<CPU>(0, 0, s % 2), c_tmp.ld());

            mpi_comm_world().ireduce(c_tmp.at<CPU>(0, 0, s % 2), c_tmp.ld() * c.num_cols_local(rank_col), blacs_grid.cart_rank(rank_row, rank_col), &req[s % 2]);
            
            s++;
        }
    }

    for (int s: {0, 1})
    {
        if (req[s] != MPI_REQUEST_NULL)
        {
            MPI_Wait(&req[s], MPI_STATUS_IGNORE);

            if (mpi_comm_world().rank() == blacs_grid.cart_rank(pos[s].first, pos[s].second))
            {
                #pragma omp parallel for
                for (int i = 0; i < c.num_cols_local(); i++)
                    std::memcpy(&c(0, i), &c_tmp(0, i, s), c.num_rows_local() * sizeof(double_complex));
            }
        }
    }

    double tval = t1.stop();
    double perf = 8e-9 * M * N * K / tval / mpi_comm_world().size();

    if (mpi_comm_world().rank() == 0)
    {
        printf("execution time (sec) : %12.6f\n", tval);
        printf("global matrix sizes: %i %i %i\n", M, N, K);
        printf("number of ranks: %i\n", mpi_comm_world().size());
        printf("performance (GFlops / rank): %12.6f\n", perf);
    }
    for (int i = 0; i < c.num_cols_local(); i++)
    {
        for (int j = 0; j < c.num_rows_local(); j++)
        {
            if (std::abs(c(j, i) - 0.01 * K) > 1e-10) TERMINATE("result is wrong");
        }
    }
    return perf;
}

double test_gemm_2(int M, int N, int K, std::vector<int> mpi_grid)
{
    runtime::Timer t("test_gemm"); 

    int bs = 32;
    BLACS_grid blacs_grid(mpi_comm_world(), mpi_grid[0], mpi_grid[1]);

    splindex<block> spl_K(K, mpi_comm_world().size(), mpi_comm_world().rank());
    
    matrix<double_complex> a(spl_K.local_size(), M);
    matrix<double_complex> b(spl_K.local_size(), N);

    dmatrix<double_complex> c(M, N, blacs_grid, bs, bs);
    c.zero();

    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j < spl_K.local_size(); j++) a(j, i) = 0.1;
    }
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < spl_K.local_size(); j++) b(j, i) = 0.1;
    }

    int BS = 256;

    mdarray<double_complex, 2> c_tmp(BS * BS, 2);
    c_tmp.zero();

    int nbr = M / BS + std::min(1, M % BS);
    int nbc = N / BS + std::min(1, N % BS);

    runtime::Timer t1("gemm_only");

    std::array<MPI_Request, 2> req = {MPI_REQUEST_NULL, MPI_REQUEST_NULL};
    std::array<std::array<int, 4>, 2> dims;
    
    int s = 0;
    for (int ibc = 0; ibc < nbc; ibc++)
    {
        int col0 = ibc * BS;
        int ncol = std::min(N, (ibc + 1) * BS) - col0;

        for (int ibr = 0; ibr < nbr; ibr++)
        {
            int row0 = ibr * BS;
            int nrow = std::min(N, (ibr + 1) * BS) - row0;

            if (req[s % 2] != MPI_REQUEST_NULL)
            {
                runtime::Timer t2("store");
                MPI_Wait(&req[s % 2], MPI_STATUS_IGNORE);
                
                #pragma omp parallel for
                for (int icol = 0; icol < dims[s % 2][3]; icol++)
                {
                    for (int irow = 0; irow < dims[s % 2][2]; irow++)
                    {
                        c.set(irow +  dims[s % 2][0], icol +  dims[s % 2][1], c_tmp(irow + dims[s % 2][2] * icol, s % 2));
                    }
                }
            }

            dims[s % 2][0] = row0;
            dims[s % 2][1] = col0;
            dims[s % 2][2] = nrow;
            dims[s % 2][3] = ncol;

            runtime::Timer t3("local_gemm");
            linalg<CPU>::gemm(2, 0, nrow, ncol, spl_K.local_size(),
                              a.at<CPU>(0, row0), a.ld(), b.at<CPU>(0, col0), b.ld(),
                              c_tmp.at<CPU>(0, s % 2), nrow);
            t3.stop();

            runtime::Timer t1("iallreduce");
            mpi_comm_world().iallreduce(c_tmp.at<CPU>(0, s % 2), nrow * ncol, &req[s % 2]);
            t1.stop();
            
            s++;
        }
    }

    for (int s: {0, 1})
    {
        if (req[s % 2] != MPI_REQUEST_NULL)
        {
            runtime::Timer t2("store");
            MPI_Wait(&req[s % 2], MPI_STATUS_IGNORE);

            #pragma omp parallel for
            for (int icol = 0; icol < dims[s % 2][3]; icol++)
            {
                for (int irow = 0; irow < dims[s % 2][2]; irow++)
                {
                    c.set(irow +  dims[s % 2][0], icol +  dims[s % 2][1], c_tmp(irow + dims[s % 2][2] * icol, s % 2));
                }
            }
        }
    }

    double tval = t1.stop();
    double perf = 8e-9 * M * N * K / tval / mpi_comm_world().size();

    if (mpi_comm_world().rank() == 0)
    {
        printf("execution time (sec) : %12.6f\n", tval);
        printf("global matrix sizes: %i %i %i\n", M, N, K);
        printf("number of ranks: %i\n", mpi_comm_world().size());
        printf("performance (GFlops / rank): %12.6f\n", perf);
    }
    for (int i = 0; i < c.num_cols_local(); i++)
    {
        for (int j = 0; j < c.num_rows_local(); j++)
        {
            if (std::abs(c(j, i) - 0.01 * K) > 1e-10) TERMINATE("result is wrong");
        }
    }
    return perf;
}

int main(int argn, char **argv)
{
    cmd_args args;
    args.register_key("--M=", "{int} M");
    args.register_key("--N=", "{int} N");
    args.register_key("--K=", "{int} K");
    args.register_key("--mpi_grid=", "{vector<int>} 2D MPI grid");
    args.register_key("--repeat=", "{int} repeat test number of times");

    args.parse_args(argn, argv);
    if (args.exist("help"))
    {
        printf("Usage: %s [options]\n", argv[0]);
        args.print_help();
        return 0;
    }

    int M = args.value<int>("M", 100);
    int N = args.value<int>("N", M);
    int K = args.value<int>("K", 1000);
    int repeat = args.value<int>("repeat", 1);
    std::vector<int> mpi_grid = args.value< std::vector<int> >("mpi_grid", {1, 1});

    sirius::initialize(true);
    
    //test_reduce(M, N, K, mpi_grid);
    //test_reduce_2(M, N, K, mpi_grid);

    double perf = 0;
    for (int i = 0; i < repeat; i++) perf += test_gemm_2(M, N, K, mpi_grid);
    if (mpi_comm_world().rank() == 0)
    {
        printf("\n");
        printf("average performance    : %12.6f GFlops / rank\n", perf / repeat);
    }

    runtime::Timer::print();

    sirius::finalize();
}
