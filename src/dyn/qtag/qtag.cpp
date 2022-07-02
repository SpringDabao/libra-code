/*********************************************************************************
* Copyright (C) 2022 Matthew Dutra, Alexey V. Akimov
*
* This file is distributed under the terms of the GNU General Public License
* as published by the Free Software Foundation, either version 3 of
* the License, or (at your option) any later version.
* See the file LICENSE in the root directory of this distribution
* or <http://www.gnu.org/licenses/>.
*
*********************************************************************************/
/**
  \file qtag.cpp
  \brief The file implements various auxiliary functions for QTAG method
    
*/

#include "qtag.h"
#include "../gwp/libgwp.h"

/// liblibra namespace
namespace liblibra{

using namespace liblinalg;

/// libdyn namespace
namespace libdyn{

using namespace libgwp;

/// libqtag namespace
namespace libqtag{


CMATRIX qtag_psi(MATRIX& q, MATRIX& q1, MATRIX& p1, MATRIX& alp1, MATRIX& s1, CMATRIX& Coeff){
/**
  Returns the (complex) wavefunction value *wf* at a given point *q* for all quantum states
  calculated using the TBF parameters stored in *q1, p1, alp1, s1* and coefficients *Coeff*.

  Args:
      q - MATRIX(ndof, 1) - point at which to evaluate the wavefunction
      q1 - MATRIX(ndof, ntraj) - coordinates of trajectories
      p1 - MATRIX(ndof, ntraj) - momenta of trajectories
      alp1 - MATRIX(ndof, ntraj) - Gaussian width parameters of trajectories
      s1 - MATRIX(ndof, ntraj) - Gaussian width parameters of trajectories
      Coeff - CMATRIX(nstates, ntraj) - amplitudes of all trajectories on all states

  Returns:
      wfc - CMATRIX(nstates, 1) - complex value of the wavefunction at q for all states

*/
  int idof, itraj, istate;
  int ndof = q1.n_rows;
  int ntraj = q1.n_cols;
  int nstates = Coeff.n_rows;

  CMATRIX wfc(nstates, 1);

  for(itraj<0; itraj<ntraj; itraj++){

    complex<double> prod(1.0, 0.0);  
    double sum1, sum2; sum1 = sum2 = 0.0;

    for(int idof=0; idof<ndof; idof++){
      double Q = q1.get(idof, itraj);
      double P = p1.get(idof, itraj);
      double A = alp1.get(idof, itraj);
      double S = s1.get(idof, itraj);
      double _q = q.get(idof, 0);
      double dq = _q - Q;

      prod *= pow((A/M_PI), 0.25);
      sum1 += A * dq * dq;
      sum2 +=  (P * dq + S);

    }// for idof

    double cs = cos(sum2);
    double si = sin(sum2);
    prod *= exp(-0.5 * sum1) * complex<double>(cs, si); 
    
    for(istate=0; istate<nstates; istate++){
      wfc.add(istate, 0,   Coeff.get(istate, itraj) * prod);
    }
  }// for itraj

  return wfc;

}


CMATRIX qtag_overlap_elementary(MATRIX& q, MATRIX& p, MATRIX& alp, MATRIX& s){
/**
  Returns a ntraj' x ntraj' matrix of the GBFs, independent of their active states

  In the gwp, we have the phase parameters first, then the Gaussian width parameters 
  also, keep in mind the 0.5 factor 

  Considering that ntraj' is the number of trajectories on a given surface

  Args:
      q - MATRIX(ndof, ntraj') - positions of the Gaussians for all trajectories
      p - MATRIX(ndof, ntraj') - momenta of the Gaussians for all trajectories
      alp - MATRIX(ndof, ntraj') - Gaussian width parameters for all trajectories
      s - MATRIX(ndof, ntraj') - Gaussian phase parameters of trajectories

  Returns:
      CMATRIX(ntraj, ntraj) - overlaps <GBF_i | GBF_j > regardless of the electronic states
    
*/

  MATRIX tmp(alp); tmp *= 0.5;

  return gwp_overlap_matrix(q, p, s, tmp, q, p, s, tmp );

}

CMATRIX qtag_kinetic_elementary(MATRIX& q, MATRIX& p, MATRIX& alp, MATRIX& s, MATRIX& invM){
/**
  Returns a ntraj' x ntraj' matrix of the kinetic energy for GBFs, independent of their active states

  Considering that ntraj' is the number of trajectories on a given surface

  In the gwp, we have the phase parameters first, then the Gaussian width parameters 
  also, keep in mind the 0.5 factor 
    
  Args:
      q - MATRIX(ndof, ntraj') - positions of the Gaussians for all trajectories
      p - MATRIX(ndof, ntraj') - momenta of the Gaussians for all trajectories
      alp - MATRIX(ndof, ntraj') - Gaussian width parameters for all trajectories
      s - MATRIX(ndof, ntraj') - Gaussian phase parameters of trajectories
      invM - MATRIX(ndof, 1) - inverse matrices for all DOFs (same across all trajectories)

  Returns:
      CMATRIX(ntraj', ntraj') - kinetic energy matrix elements <GBF_i | T | GBF_j > regardless of the electronic states

*/

  MATRIX tmp(alp); tmp *= 0.5;

  return gwp_kinetic_matrix(q, p, s, tmp, q, p, s, tmp, invM );

}



CMATRIX qtag_overlap(vector<int>& active_states, CMATRIX& ovlp, int nstates){
/**
  Calculates the single-surface Hamiltonian matrix elements H_ij=<gi|KE+V|gj>, 
  computed using the basis parameters stored in *qpas*. This requires the single-surface 
  overlap matrix *ov*. Returns the single-surface Hamiltonian *H*.

  Args:
    active_states - list[ ntraj x int] - state indices for all trajectories
    ovlp - CMATRIX(ntraj, ntraj) - couplings of all trajectories as if they are on the same state

  Returns:
    S - CMATRIX(nstates x ntraj, nstates x ntraj) - The many-surface Hamiltonian matrix.

*/
  int i, j;
  int ntraj = active_states.size();

  CMATRIX S(nstates * ntraj, nstates * ntraj);

  for(int itraj=0; itraj<ntraj; itraj++){
    //for(int i=0; i<nstates; i++){
    i = active_states[itraj];

      int indx1 = itraj * nstates + i;

      for(int jtraj=0; jtraj<ntraj; jtraj++){
        //for(int j=0; j<nstates; j++){
          j = active_states[jtraj];
 
          int indx2 = jtraj * nstates + j;

          complex<double> val(0.0, 0.0);
          if(active_states[itraj] == active_states[jtraj]){
            val = ovlp.get(itraj, jtraj);
          }

          S.set(indx1, indx2, val);
        
//        }// j
      }// jtraj

//    }// for i
  }// for itraj

  return S;
}



complex<double> BAT(CMATRIX* Ham1, CMATRIX* Ham2, vector<CMATRIX*>& dHam1, vector<CMATRIX*>& dHam2,
                    MATRIX& q1, MATRIX& p1, MATRIX& s1, MATRIX& alp1, int n1, 
                    MATRIX& q2, MATRIX& p2, MATRIX& s2, MATRIX& alp2, int n2){
/**
    """Returns the (complex) value for the potential *v* on an energetic surface specified by *nsurf* from two basis 
       functions defined by their parameters *qpasi* and *qpasj*, respectively. The computation employs the Bra-ket 
       Averaged Taylor expansion (BAT) about each basis center, which requires a potential function *pot* as well as 
       its first derivative. The potential parameters are stored in the dict 'model_params' as defined in qtag_config.

    Args:

        nstate (integer): Integer specifying the potential surface to be calculated (0 = ground, 1 = first excited, ...)

        qpasi (list): The ndof-by-4 parameter list of the i-th basis function. Each entry is an ndof-by-1 column MATRIX.

        qpasj (list): The ndof-by-4 parameter list of the j-th basis function. Each entry is an ndof-by-1 column MATRIX.

        params (dictionary): Dictionary containing the potential parameters.

        libra_model (function object): Function object containing the Libra potential model for the ground and excited states.

    Returns:
        complex : v - The complex potential v computed via the bra-ket averaged Taylor expansion between basis functions i and j.

    """
*/

  //cout<<"In BAT\n";

  complex<double> vx1, vx2, dvx1, dvx2, v;
  int ndof = q1.n_rows;

  vx1 = Ham1->get(n1, n2);
  vx2 = Ham2->get(n1, n2);

  v = 0.5 * (vx1 + vx2);

  //cout<<"average pot = "<<v<<endl;

  for(int dof=0; dof<ndof; dof++){

    dvx1 = dHam1[dof]->get(n1,n2);
    dvx2 = dHam2[dof]->get(n1,n2);

    double q1i = q1.get(dof);
    double q2i = q2.get(dof);

    double p1i = p1.get(dof);
    double p2i = p2.get(dof);

    double a1i = alp1.get(dof);
    double a2i = alp2.get(dof);

    double dq = q2i - q1i;
    double dp = p2i - p1i;
    double denom = a1i + a2i; 

    complex<double> q1_rr1_q2(a2i*dq, dp); 
    complex<double> q1_rr2_q2(-a1i*dq, dp);

    //cout<<"dof = "<<dof<<" a1= "<<a1i<<" a2= "<<a2i<<endl;
    //cout<<"dof = "<<dof<<" denom = "<<denom<<" dvx1 = "<<dvx1<<" q1_rr1_q2 = "<<q1_rr1_q2<<" dvx2 = "<<dvx2<<" q1_rr2_q2= "<<q1_rr2_q2<<endl;

    v += 0.5 * (dvx1*q1_rr1_q2 + dvx2*q1_rr2_q2)/denom;

  }// for  dof

  return v;

}// BAT



complex<double> LHA(CMATRIX* Ham1, CMATRIX* Ham2, 
                    vector<CMATRIX*>& dHam1, vector<CMATRIX*>& dHam2,
                    vector<CMATRIX*>& d2Ham1, vector<CMATRIX*>& d2Ham2,
                    MATRIX& q1, MATRIX& p1, MATRIX& s1, MATRIX& alp1, int n1, 
                    MATRIX& q2, MATRIX& p2, MATRIX& s2, MATRIX& alp2, int n2){
/**
    """Returns the (complex) value for the potential *v* on an energetic surface specified by *nsurf* from two basis 
       functions defined by their parameters *qpasi* and *qpasj*, respectively. The computation employs the Local Harmonic 
       Approximation (LHA), which requires a potential function *pot* as well as its first and second derivatives. 
       The potential parameters are stored in the dict 'model_params' as defined in qtag_config.

    Args:
        univ (dictionary): Dictionary containing various system parameters.

        nstate (integer): Integer specifying the potential surface to be calculated (0 = ground, 1 = first excited, ...)

        qpasi (list): The ndof-by-4 parameter list of the i-th basis function. Each entry is an ndof-by-1 column MATRIX.

        qpasj (list): The ndof-by-4 parameter list of the j-th basis function. Each entry is an ndof-by-1 column MATRIX.

        params (dictionary): Dictionary containing the potential parameters.

        libra_model (function object): Function object containing the Libra potential model for the ground and excited states.

    Returns:
        v (complex): The complex potential v computed via the local harmonic approximation between basis functions i and j.
    """

*/

  complex<double> vx1, vx2, dvx1, dvx2, d2vx1, d2vx2, v;
  int ndof = q1.n_rows;

  vx1 = Ham1->get(n1, n2);
  vx2 = Ham2->get(n1, n2);

  v = 0.5 * (vx1 + vx2);


  for(int dof=0; dof<ndof; dof++){

    dvx1 = dHam1[dof]->get(n1,n2);
    dvx2 = dHam2[dof]->get(n1,n2);

    d2vx1 = d2Ham1[dof]->get(n1,n2);
    d2vx2 = d2Ham2[dof]->get(n1,n2);


    double q1i = q1.get(dof);
    double q2i = q2.get(dof);

    double p1i = p1.get(dof);
    double p2i = p2.get(dof);

    double a1i = alp1.get(dof);
    double a2i = alp2.get(dof);

    double dq = q2i - q1i;
    double dp = p2i - p1i;
    double denom = a1i + a2i; 

    double aqp = a1i * q1i + a2i * q2i;

    complex<double> z(aqp, dp); z = z/denom;


    complex<double> vv01 = -dvx1*q1i + 0.5*d2vx1*q1i*q1i;
    complex<double> vv02 = -dvx2*q2i + 0.5*d2vx2*q2i*q2i;

    v += 0.5 * (vv01 + vv02);

    complex<double> vv11 = -d2vx1*q1i + dvx1;
    complex<double> vv12 = -d2vx2*q2i + dvx2;

    v += 0.5 * (vv11 + vv12)*z;

    v += 0.25 * (d2vx1 + d2vx2) * (z*z + 1.0/denom);

  }// for  dof

  return v;

}// LHA

complex<double> LHAe(int i, int j, 
                     MATRIX& q1, MATRIX& p1, MATRIX& s1, MATRIX& alp1, int n1,
                     MATRIX& q2, MATRIX& p2, MATRIX& s2, MATRIX& alp2, int n2,
                     nHamiltonian& ham){
/**
    """Returns the (complex) value for the potential *v* on an energetic surface specified by *nsurf* from two basis
       functions defined by their parameters *qpasi* and *qpasj*, respectively. The computation employs the Local Harmonic
       Approximation (LHA), which requires a potential function *pot* as well as its first and second derivatives.
       The potential parameters are stored in the dict 'model_params' as defined in qtag_config.

    Args:
        univ (dictionary): Dictionary containing various system parameters.

        nstate (integer): Integer specifying the potential surface to be calculated (0 = ground, 1 = first excited, ...)

        qpasi (list): The ndof-by-4 parameter list of the i-th basis function. Each entry is an ndof-by-1 column MATRIX.

        qpasj (list): The ndof-by-4 parameter list of the j-th basis function. Each entry is an ndof-by-1 column MATRIX.

        params (dictionary): Dictionary containing the potential parameters.

        libra_model (function object): Function object containing the Libra potential model for the ground and excited states.

    Returns:
        v (complex): The complex potential v computed via the local harmonic approximation between basis functions i and j.
    """
*/

  complex<double> v;
  int ndof = q1.n_rows;

//Holstein coupling
  double A = 1.0;
  double B = 1.5811;
  double C = 2.0;

//Tully1 coupling
//  double A = 0.005;
//  double B = 1.0;
//  double C = 0.0;

//Tully2 coupling
//  double A = 0.015;
//  double B = 0.06;
//  double C = 0.0;

  if(n1==n2){// LHA for single-surface elements

    v = LHA(ham.children[i]->ham_dia, ham.children[j]->ham_dia,
            ham.children[i]->d1ham_dia, ham.children[j]->d1ham_dia,
            ham.children[i]->d2ham_dia, ham.children[j]->d2ham_dia,
            q1, p1, s1, alp1, n1, q2, p2, s2, alp2, n2);

  }
  else if(n1!=n2){// Exact integral for Gaussian coupling

    v = (0.0, 0.0);

    for(int dof=0; dof<ndof; dof++){

      double q1i = q1.get(dof);
      double q2i = q2.get(dof);

      double p1i = p1.get(dof);
      double p2i = p2.get(dof);

      double a1i = alp1.get(dof);
      double a2i = alp2.get(dof);

      double dCq1 = C - q1i;
      double dCq2 = C - q2i;

      double aCq1 = dCq1*a1i;
      double aCq2 = dCq2*a2i;

      double dp = p1i - p2i;
      double as = a1i + a2i;
      double aB = a1i + 2*B + a2i;
      

      double prefac1 = A*sqrt(as)/sqrt(aB);
      double prefac2 = -B/(aB*as);

      complex<double> expt(aCq1*aCq1+aCq2*aCq2-dp*dp+2.0*aCq1*aCq2, 2.0*dp*(aCq1+aCq2));
      v += prefac1*exp(prefac2*expt);
    }// for  dof
  }//end if

  return v;

}//LHAe

complex<double> BATe(int i, int j,
                     MATRIX& q1, MATRIX& p1, MATRIX& s1, MATRIX& alp1, int n1,
                     MATRIX& q2, MATRIX& p2, MATRIX& s2, MATRIX& alp2, int n2,
                     nHamiltonian& ham){
/**
    """Returns the (complex) value for the potential *v* on an energetic surface specified by *nsurf* from two basis
       functions defined by their parameters *qpasi* and *qpasj*, respectively. The computation employs the Local Harmonic
       Approximation (LHA), which requires a potential function *pot* as well as its first and second derivatives.
       The potential parameters are stored in the dict 'model_params' as defined in qtag_config.

    Args:
        univ (dictionary): Dictionary containing various system parameters.

        nstate (integer): Integer specifying the potential surface to be calculated (0 = ground, 1 = first excited, ...)

        qpasi (list): The ndof-by-4 parameter list of the i-th basis function. Each entry is an ndof-by-1 column MATRIX.

        qpasj (list): The ndof-by-4 parameter list of the j-th basis function. Each entry is an ndof-by-1 column MATRIX.

        params (dictionary): Dictionary containing the potential parameters.

        libra_model (function object): Function object containing the Libra potential model for the ground and excited states.

    Returns:
        v (complex): The complex potential v computed via the local harmonic approximation between basis functions i and j.
    """
*/

  complex<double> v;
  int ndof = q1.n_rows;

//Holstein coupling
  double A = 1.0;
  double B = 1.5811;
  double C = 2.0;

//Tully1 coupling
//  double A = 0.005;
//  double B = 1.0;
//  double C = 0.0;

//Tully2 coupling
//  double A = 0.015;
//  double B = 0.06;
//  double C = 0.0;

  if(n1==n2){// BAT for single-surface elements

    v = BAT(ham.children[i]->ham_dia, ham.children[j]->ham_dia,
            ham.children[i]->d1ham_dia, ham.children[j]->d1ham_dia,
            q1, p1, s1, alp1, n1, q2, p2, s2, alp2, n2);

  }
  else if(n1!=n2){// Exact integral for Gaussian coupling

    v = (0.0, 0.0);

    for(int dof=0; dof<ndof; dof++){

      double q1i = q1.get(dof);
      double q2i = q2.get(dof);

      double p1i = p1.get(dof);
      double p2i = p2.get(dof);

      double a1i = alp1.get(dof);
      double a2i = alp2.get(dof);

      double dCq1 = C - q1i;
      double dCq2 = C - q2i;

      double aCq1 = dCq1*a1i;
      double aCq2 = dCq2*a2i;

      double dp = p1i - p2i;
      double as = a1i + a2i;
      double aB = a1i + 2*B + a2i;


      double prefac1 = A*sqrt(as)/sqrt(aB);
      double prefac2 = -B/(aB*as);

      complex<double> expt(aCq1*aCq1+aCq2*aCq2-dp*dp+2.0*aCq1*aCq2, 2.0*dp*(aCq1+aCq2));
      v += prefac1*exp(prefac2*expt);
    }// for  dof
  }//end if

  return v;

}//BATe

CMATRIX qtag_potential(MATRIX& q1, MATRIX& p1, MATRIX& s1, MATRIX& alp1, int n1, vector<int>& traj_on_surf_n1,
                       MATRIX& q2, MATRIX& p2, MATRIX& s2, MATRIX& alp2, int n2, vector<int>& traj_on_surf_n2,
                       nHamiltonian& ham, int method){

  int ntraj_on_surf_n1 = q1.n_cols;
  int ntraj_on_surf_n2 = q2.n_cols;
  int ndof = q1.n_rows;

  complex<double> v;
  CMATRIX res(ntraj_on_surf_n1, ntraj_on_surf_n2);
  MATRIX qi(ndof, 1);
  MATRIX pi(ndof, 1);
  MATRIX ai(ndof, 1);
  MATRIX si(ndof, 1);
  MATRIX qj(ndof, 1);
  MATRIX pj(ndof, 1);
  MATRIX aj(ndof, 1);
  MATRIX sj(ndof, 1);


  for(int itraj=0; itraj<ntraj_on_surf_n1; itraj++){

    int i = traj_on_surf_n1[itraj];

    qi = q1.col(itraj);
    pi = p1.col(itraj);
    ai = alp1.col(itraj);
    si = s1.col(itraj);

    for(int jtraj=0; jtraj<ntraj_on_surf_n2; jtraj++){

      int j = traj_on_surf_n2[jtraj];

      qj = q2.col(jtraj);
      pj = p2.col(jtraj);
      aj = alp2.col(jtraj);
      sj = s2.col(jtraj);


      if(method==0){ // BAT

        v = BAT(ham.children[i]->ham_dia, ham.children[j]->ham_dia, 
                ham.children[i]->d1ham_dia, ham.children[j]->d1ham_dia,
                qi, pi, si, ai, n1, qj, pj, sj, aj, n2);

      }// BAT
      else if(method==1){// LHA
        v = LHA(ham.children[i]->ham_dia, ham.children[j]->ham_dia, 
                ham.children[i]->d1ham_dia, ham.children[j]->d1ham_dia,
                ham.children[i]->d2ham_dia, ham.children[j]->d2ham_dia,
                qi, pi, si, ai, n1, qj, pj, sj, aj, n2);
      }// LHA
      else if(method==2){// LHAe
        v = LHAe(i, j, qi, pi, si, ai, n1, qj, pj, sj, aj, n2, 
                 ham);
      }// LHAe
      else if(method==3){// BATe
        v = BATe(i, j, qi, pi, si, ai, n1, qj, pj, sj, aj, n2,
                 ham);
      }// BATe

      res.set(itraj, jtraj, v);

    }// for j
  }// for i

  return res;

}

void qtag_hamiltonian_and_overlap(MATRIX& q, MATRIX& p, MATRIX& alp, MATRIX& s, CMATRIX& Coeff,
                                  vector<int>& active_states, MATRIX& invM, 
                                  nHamiltonian& ham, bp::object compute_ham_funct, bp::dict compute_ham_params,
                                  bp::dict& dyn_params,
                                  CMATRIX& super_ovlp, CMATRIX& super_ham){
/**
  Calculates the single-surface Hamiltonian matrix elements H_ij=<gi|KE+V|gj>, 
  computed using the basis parameters stored in *qpas*. This requires the single-surface 
  overlap matrix *ov*. Returns the single-surface Hamiltonian *H*.

  Args:
    q - MATRIX(ndof, ntraj) - coordinates of the system
    p - MATRIX(ndof, ntraj) - momenta of the system
    alp - MATRIX(ndof, ntraj) - widths
    s - MATRIX(ndof, ntraj) - phases
    active_states - list[ ntraj x int] - state indices for all trajectories
    invM - MATRIX(ndof, 1) - inverse masses for all DOFs
    ham - the Hamiltonian object that knows how to compute interactions for independent trajectories
    compute_ham_funct (PyObject) - the Python function that computes the model Hamiltonian
    compute_ham_params ( dict ) - the dictionary for the function above.
    dyn_params ( dict ) - parameters to control execution of this function
    super_ovlp ( CMATRIX(ntraj, ntraj) ) - the results for the super-overlap are stored here
    super_ham ( CMATRIX(ntraj, ntraj) ) - the results for the super-Hamiltonian are stored here

  Returns:

*/

//  exit(0);

  dyn_control_params prms;
  prms.set_parameters(dyn_params);

  int method = prms.qtag_pot_approx_method; 

//  int method = 0;

  int ndof = q.n_rows;
  int ntraj = q.n_cols;
  int nstates = Coeff.n_rows;
  //int ntraj = active_states.size();
  int i, j, itraj, n1, n2, indx;

//  cout<<"ndof= "<<ndof<<" nstates= "<<nstates<<" ntraj= "<<ntraj<<endl;

  vector<int> dof_dim(ndof); for(i=0;i<ndof;i++){ dof_dim[i] = i; }
  vector< vector<int> > traj_on_surf(nstates); // indices of trajectories on each state


  indx = 0;
  n1 = active_states[0];
  for(itraj = 0; itraj<ntraj; itraj++){
    if(n1 != active_states[itraj]){
      indx+=1;
      }//if n1 != active_states[itraj]

    traj_on_surf[indx].push_back(itraj);
    n1 = active_states[itraj];
  }// for itraj


  // Compute Hamiltonians for all the trajectories
  ham.compute_diabatic(compute_ham_funct, bp::object(q), compute_ham_params, 1);

//  exit(0);

  // State blocks
  for(n1=0; n1<nstates; n1++){

    int ntraj_on_surf_n1 = traj_on_surf[n1].size();

    if(ntraj_on_surf_n1 > 0){

      MATRIX q1(ndof, ntraj_on_surf_n1);
      MATRIX p1(ndof, ntraj_on_surf_n1);
      MATRIX a1(ndof, ntraj_on_surf_n1);
      MATRIX s1(ndof, ntraj_on_surf_n1);

      pop_submatrix(q,   q1, dof_dim, traj_on_surf[n1]);
      pop_submatrix(p,   p1, dof_dim, traj_on_surf[n1]);
      pop_submatrix(alp, a1, dof_dim, traj_on_surf[n1]);
      pop_submatrix(s,   s1, dof_dim, traj_on_surf[n1]);

      MATRIX a1_half(a1); a1_half *= 0.5;


      for(n2=n1; n2<nstates; n2++){

        int ntraj_on_surf_n2 = traj_on_surf[n2].size();

        if(ntraj_on_surf_n2 > 0){

          MATRIX q2(ndof, ntraj_on_surf_n2);
          MATRIX p2(ndof, ntraj_on_surf_n2);
          MATRIX a2(ndof, ntraj_on_surf_n2);
          MATRIX s2(ndof, ntraj_on_surf_n2);

          pop_submatrix(q,   q2, dof_dim, traj_on_surf[n2]);
          pop_submatrix(p,   p2, dof_dim, traj_on_surf[n2]);
          pop_submatrix(alp, a2, dof_dim, traj_on_surf[n2]);
          pop_submatrix(s,   s2, dof_dim, traj_on_surf[n2]);

          MATRIX a2_half(a2); a2_half *= 0.5;


          //=================== Main calculations ===============

          // Overlap 
          CMATRIX s12(ntraj_on_surf_n1, ntraj_on_surf_n2);

          s12 = gwp_overlap_matrix(q1, p1, s1, a1_half, q2, p2, s2, a2_half);

          if(n1==n2){
             push_submatrix(super_ovlp, s12, traj_on_surf[n1], traj_on_surf[n2]);
          }

          // Hamiltonian 
          CMATRIX h12(ntraj_on_surf_n1, ntraj_on_surf_n2);
          CMATRIX pot(ntraj_on_surf_n1, ntraj_on_surf_n2);
          
          pot = qtag_potential(q1, p1, s1, a1, n1, traj_on_surf[n1], q2, p2, s2, a2, n2, traj_on_surf[n2], ham, method);
          h12.dot_product(pot, s12);

          if(n1==n2){ // kinetic energy for the diagonal terms
            CMATRIX kin(ntraj_on_surf_n1, ntraj_on_surf_n1);
            kin = gwp_kinetic_matrix(q1, p1, s1, a1_half, q2, p2, s2, a2_half, invM );
            h12 += kin;
          }

          push_submatrix(super_ham, h12, traj_on_surf[n1], traj_on_surf[n2]);
          if(n1 != n2){
            CMATRIX h21(ntraj_on_surf_n2, ntraj_on_surf_n1);  h21 = h12.H();
            push_submatrix(super_ham, h21, traj_on_surf[n2], traj_on_surf[n1]);
          }


        }// if ntraj_on_surf_n2 > 0     
      }// for n2

    }// if ntraj_on_surf_n1 > 0     
  }// for n1

}



CMATRIX qtag_momentum(MATRIX& q, MATRIX& p, MATRIX& alp, MATRIX& s, CMATRIX& Coeff){
/**
  Returns the momentum *mom* calculated for each basis function according to p=Im(grad(psi)/psi). 
  Also returns the corresponding real component *r*, which can be used in updates of the basis phase parameter *s*.

*/


  int i,j, idof;
  int ndof = q.n_rows;
  int ntraj = q.n_cols;
  complex<double> dzt;
  double dq, nrm;

  CMATRIX mom(ndof,ntraj); // real = mom, imag = r
  CMATRIX dz(ndof,1);
  complex<double> one(0.0, 1.0);

  for(i=0; i<ntraj; i++){
    complex<double> z(0.0,0.0);
    dz *= 0.0;

    for(j=0; j<ntraj; j++){
      complex<double> nrm(1.0, 0.0);

      for(idof=0; idof<ndof; idof++){
          dq = q.get(idof, i) - q.get(idof, j);
          double argg = (p.get(idof, j) * dq + s.get(idof, j) );
          complex<double> tmp( cos(argg), sin(argg) );
          nrm *= exp( -0.5 * alp.get(idof, j) * dq*dq ) * tmp * pow( (alp.get(idof, i)/M_PI), 0.25);
     
      }// for idof

      z += Coeff.get(j) * nrm;

      for(idof=0; idof<ndof; idof++){
        dq = q.get(idof, i) - q.get(idof, j);
        dzt = -(alp.get(idof, j) * dq - one * p.get(idof, j)) * Coeff.get(j) * nrm;
        dz.add(idof, 0, dzt);
      }// for idof
      
    }// for j

    for(idof=0; idof<ndof; idof++){
      mom.set(i, idof,  dz.get(idof)/z );
    }// for idof

  }// for i

  return mom;

}



}// namespace libqtag
}// namespace libdyn
}// liblibra


