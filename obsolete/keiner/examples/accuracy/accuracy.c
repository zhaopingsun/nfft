/* 
   accuracy - Accuracy test for the NFSFT

   Copyright (C) 2005 Jens Keiner

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  

#include <termios.h>
#include <grp.h>
#include <pwd.h>
*/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#else
#  error Need config.h
#endif

#ifdef STDC_HEADERS
#  include <stdlib.h>
#  include <stdio.h>
#  include <math.h>
#else
#  error Need ANSI-C headers
#endif

/* Auxilliary headers */
#include <complex.h>
#include "nfsft.h"
#include "util.h"
#include "time.h"

#define MAX_BANDWIDTH_EXPONENT 8

/**
 * The main program.
 *
 * \param argc The number of arguments
 * \param argv An array containing the arguments as C-strings
 *
 * \return Exit code 
 */
int main (int argc, char **argv)
{  
  /** Maximum bandwidth */
  const int M_MAX = 1<<MAX_BANDWIDTH_EXPONENT;
  /** Next greater power of 2 relative to M */
  const int N_MAX = 1<<ngpt(M_MAX);
  /** Maximum number of nodes */
  const int D_MAX = (M_MAX+1)*(2*M_MAX+2);
  
  /** The current bandwidth */
  int M;
  /** Next greater power of two relative to M */
  int N;
  /** Loop counter for Legendre index k. */
  int k;
  /** Loop counter for Legendre index n. */
  int n;
  /** The current number of nodes */
  int D;
  /** Loop counter for nodes. */
  int d;  
  
  /** Arrays for complex Fourier coefficients. */
  complex **f_hat;
  /** Copy the original Fourier coefficients. */
  complex **f_hat_orig;
  
  /** Array of angles defining the nodes. */
  double *angles;
  /** Array for angles \f$\theta\f$ of a grid. */
  double *theta;
  /** Array for angles \f$\phi\f$ of a grid. */
  double* phi;
  /** Array for Gauss-Legendre weights. */
  double *w;  
  /** Array for function values. */
  complex *f;
  
  /** Plan for fast spherical fourier transform. */
  nfsft_plan plan;

  /** Used to measure computation time. */
  //double ctime;
  /** 
   * Used to store the filename of a file containing Gauss-Legendre nodes and 
   * weights.
   */ 
  char filename[20];
  /** File handle for reading quadrature nodes and weights. */
  FILE *file;
  
  /* Initialize random number generator. */
  srand48(time(NULL));
    
  /* Allocate memory. */
  f_hat = (complex**) malloc((2*M_MAX+1)*sizeof(complex*));
  f_hat_orig = (complex**) malloc((2*M_MAX+1)*sizeof(complex*));
  for (n = -M_MAX; n <= M_MAX; n++)
  {
    f_hat[n+M_MAX] = (complex*) malloc((N_MAX+1)*sizeof(complex));
    f_hat_orig[n+M_MAX] = (complex*) malloc((N_MAX+1)*sizeof(complex));
  }  
  
  angles = (double*) malloc(2*D_MAX*sizeof(double));
  theta = (double*) malloc((M_MAX+1)*sizeof(double));
  phi = (double*) malloc((2*M_MAX+2)*sizeof(double));
  w = (double*) malloc((M_MAX+1)*sizeof(double));
  f = (complex*) malloc(2*D_MAX*sizeof(complex));
  
  nfsft_compute_wisdom(M_MAX);
  
  /* Test backward stability of NDSFT. */
  for (M = 8/*1*/; M <= 256/*< M_MAX*/; M = M+8/*M*=2*/)
  {
    /* Perform backward stability test:
     * 1. For \f$k = 0,...,M\f$ and \f$n = -k,...,k\f$ compute random Fourier 
     *    coefficients \f$a_k^n \in \[0,1\] x i \[0,1\]\f$.
     * 2. Evaluate \f$f := \sum_{k=0}^M \sum_{n=-k}^n a_k^n Y_k^n\f$ at 
     *    Gauss-Legendre nodes \f$\left(\xi_d\right)_{d=0}^{(M+1)^2-1}\f$ by a
     *    NDSFT.
     * 3. Multiply each function value \f$f_d := f\left(\xi_d\right)\f$ by it's
     *    corresponding Gauss-Legendre weight w_d.
     * 4. Compute the Fourier coefficients \f$\tilde{a}_k^n\$f by Gauss-Legendre
     *    quadrature rule using a adjoint NDSFT.
     */
    
    /* Compute next greater power of two. */
    N = 1<<ngpt(M);
    
    /* Compute number of nodes. */
    D = (M+1)*(2*M+2);
    
    /* Compute random Fourier coefficients. */
    for (n = -M; n <= M; n++)
    {
      for (k = 0; k < abs(n); k++)
      {
        /* These are zero by definition. */
        f_hat[n+M][k] = 0.0;
      }
      for (k = abs(n); k <= M; k++)
      {
        f_hat[n+M][k] = drand48() + I*drand48();
        /* Save a copy. */
        f_hat_orig[n+M][k] = f_hat[n+M][k];
      }
      for (k = M+1; k <= N; k++)
      {
        /* These are zero by definition. */
        f_hat[n+M][k] = 0.0;
      }
    }

    /* Respect normalization. */
    for (n = -M; n <= M; n++)
    {
      for (k = abs(n); k <= M; k++)
      {
        f_hat[n+M][k] *= sqrt((2*k+1)/2.0);
      }
    }
       
    /* Read Gauss-Legendre nodes and weights. */  
    sprintf(filename,"gl%d.dat",M);  
    file = fopen(filename,"r");
    for (n = 0; n < M+1; n++)
    {
      fscanf(file,"%lf\n",&theta[n]);
    }
    for (k = 0; k < 2*M+2; k++)
    {
      fscanf(file,"%lf\n",&phi[k]);
    }
    for (n = 0; n < M+1; n++)
    {
      fscanf(file,"%lf\n",&w[n]);
    }    
    fclose(file);
    
    /* Create grid nodes. */
    d = 0;
    for (n = 0; n < M+1; n++)
    {
      for (k = 0; k < 2*M+2; k++)
      {
        angles[2*d] = phi[k];
        angles[2*d+1] = theta[n];
        d++;
      }  
    }
        
    /* Compute forward transform. */
    plan = nfsft_init(D, M, angles, f_hat, f, 0U);
    //ndsft_trafo(plan);
    nfsft_trafo(plan);
    nfsft_finalize(plan);
        
    /* Multiply with quadrature weights. */
    d = 0;
    for (n = 0; n < M+1; n++)
    {
      for (k = 0; k < 2*M+2; k++)
      {
        f[d] *= w[n];
        d++;
      }  
    }
    
    /* Compute adjoint transform. */
    plan = nfsft_init(D, M, angles, f_hat, f, 0U);
    //ndsft_adjoint(plan);
    nfsft_adjoint(plan);
    nfsft_finalize(plan);
    
    /* Respect normalization. */
    for (n = -M; n <= M; n++)
    {
      for (k = abs(n); k <= M; k++)
      {
        f_hat[n+M][k] *= (1.0/(2*M+2))*sqrt((2*k+1)/2.0);
      }
    }

    /* Print relative error in infinity-norm. */    
    fprintf(stdout,"%d: %20.16E\n",M,error_f_hat(f_hat_orig,f_hat,M));    
  }    
  return EXIT_SUCCESS;
}