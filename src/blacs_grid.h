#ifndef __BLACS_GRID_H__
#define __BLACS_GRID_H__

#include "mpi_grid.h"

class BLACS_grid
{
    private:

        Communicator comm_;

        MPI_grid mpi_grid_;

        int num_ranks_row_;

        int num_ranks_col_;

        int rank_row_;

        int rank_col_;
        
        int blacs_handler_;

        int blacs_context_;

        BLACS_grid(BLACS_grid const& src) = delete;
        BLACS_grid& operator=(BLACS_grid const& src) = delete; 

    public:
        
        BLACS_grid(Communicator const& comm__, int num_ranks_row__, int num_ranks_col__)
            : comm_(comm__),
              num_ranks_row_(num_ranks_row__),
              num_ranks_col_(num_ranks_col__)
        {
            std::vector<int> xy(2);
            xy[0] = num_ranks_col__;
            xy[1] = num_ranks_row__;

            mpi_grid_ = MPI_grid(xy, comm__);

            rank_col_ = mpi_grid_.coordinate(0);
            rank_row_ = mpi_grid_.coordinate(1);

            /* create handler first */
            blacs_handler_ = linalg<scalapack>::create_blacs_handler(comm_.mpi_comm());

            mdarray<int, 2> map_ranks(num_ranks_row__, num_ranks_col__);
            for (int i = 0; i < num_ranks_row__; i++)
            {
                for (int j = 0; j < num_ranks_col__; j++)
                {
                    xy[0] = j;
                    xy[1] = i;
                    map_ranks(i, j) = mpi_grid_.communicator().cart_rank(xy);
                }
            }

            /* create context */
            blacs_context_ = blacs_handler_;
            linalg<scalapack>::gridmap(&blacs_context_, map_ranks.ptr(), map_ranks.ld(), num_ranks_row__, num_ranks_col__);

            /* check the grid */
            int nrow1, ncol1, irow1, icol1;
            linalg<scalapack>::gridinfo(blacs_context_, &nrow1, &ncol1, &irow1, &icol1);

            if (rank_row_ != irow1 || rank_col_ != icol1 || num_ranks_row__ != nrow1 || num_ranks_col__ != ncol1) 
            {
                std::stringstream s;
                s << "wrong grid" << std::endl
                  << "            row | col | nrow | ncol " << std::endl
                  << " mpi_grid " << rank_row_ << " " << rank_col_ << " " << num_ranks_row__ << " " << num_ranks_col__ << std::endl  
                  << " blacs    " << irow1 << " " << icol1 << " " << nrow1 << " " << ncol1;
                error_local(__FILE__, __LINE__, s);
            }
        }

        ~BLACS_grid()
        {
            linalg<scalapack>::gridexit(blacs_context_);
            linalg<scalapack>::free_blacs_handler(blacs_handler_);
        }

        inline int context() const
        {
            return blacs_context_;
        }

        inline Communicator const& comm() const
        {
            return comm_;
        }

        inline Communicator const& comm_row() const
        {
            return mpi_grid_.communicator(1 << 1);
        }

        inline Communicator const& comm_col() const
        {
            return mpi_grid_.communicator(1 << 0);
        }

        inline int num_ranks_row() const
        {
            return num_ranks_row_;
        }

        inline int rank_row() const
        {
            return rank_row_;
        }

        inline int num_ranks_col() const
        {
            return num_ranks_col_;
        }

        inline int rank_col() const
        {
            return rank_col_;
        }
};

#endif // __BLACS_GRID_H__
