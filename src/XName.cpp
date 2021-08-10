#include "XName.h"

using namespace Eigen;
using namespace Rcpp;
using namespace std;


XName::XName(const List& data,
             const List& ctrl,
             const int G_,
             const int R_,
             const VectorXd& n_r,
             const std::vector<IntegerVector>& Races,
             const String type
               
) :
  W(as< std::vector<IntegerVector> >(data["record_name_id"])),
  phi_tilde(as<MatrixXd>(data["census_table"])),
  gamma_prior(as<VectorXd>(ctrl["gamma_prior"])),
  beta_w(as<double>(ctrl["beta_prior"])),
  N_(as<int>(data["n_unique_names"])),
  R_(R_),
  n_r(n_r),
  type(type)
{
  //Initialize containers
  max_kw = as<int>(data["largest_keyword"]);
  n_rc.resize(R_, 2);
  n_rc.setZero();
  n_wr.resize(N_, R_);
  n_wr.setZero();
  int geo_size = 0, r_ = 0, w_ = 0;

  
  for(int ii = 0; ii < G_ ; ++ii){ //iterate over geos
    geo_size =  W[ii].size();
    C.push_back(VectorXi(geo_size));
    C[ii].setZero(geo_size);
  }
  
  
  e_phi.resize(N_, R_);
  e_phi.setZero();
  
  //Initialize placeholders
  c1_prob = 1.0; numerator = 0.0;
  denominator = 1.0; c0_prob = 1.0;
  sum_c = 1.0; pi_0 = 1.0; pi_1 = 1.0;
  c = 0; new_c = 0; r_ = 0; w_ = 0; 
  
}

void XName::sample_c(int r,
                     int voter,
                     int geo_id)
{
  //Get unique names...
  w_ = W[geo_id][voter];
  //(Check if name is in keyword list, and skip update if not)
//if(!found_in(keywords[r], w_)){	
  if(w_ >= max_kw){	
    return;
  }
  
  // ... and current mixture component
  c = C[geo_id][voter];
  
  // remove data
  if(c == 0){
    n_wr(w_, r)--;
  }
  n_rc(r, c)--;
  
  // newprob_c0
  c1_prob = phi_tilde(w_, r) * (n_rc(r, 1) + gamma_prior[0]);
  
  // newprob_c0
  numerator = (n_wr(w_, r) + beta_w)
    * (n_rc(r, 0) + gamma_prior[1]);
  denominator = n_rc(r, 0) + ((double)N_ * beta_w);
  c0_prob = numerator / denominator;
  
  
  // Normalize
  sum_c = c0_prob + c1_prob;
  
  c1_prob = c1_prob / sum_c;
  new_c = (R::runif(0,1) <= c1_prob);  //new_c = Bern(c1_prob);
  
  // add back data counts
  if (new_c == 0) {
    n_wr(w_, r)++;
  }
  n_rc(r, new_c)++;
  
  //Update mixture component
  C[geo_id][voter] = new_c;
}

void XName::phihat_store(){
  // keyword component
  for(int r = 0; r < R_; ++r){
    denominator =  n_rc(r, 1) + n_rc(r, 0) + gamma_prior.sum();
    denominator_phi = n_r(r) + ((double)N_ * beta_w);
    pi_0 = (n_rc(r, 1) + gamma_prior[0]) / denominator;
    pi_1 = (n_rc(r, 0) + gamma_prior[1]) / denominator;
    for(int w = 0; w < N_; ++w){
      if(w < max_kw){
      e_phi(w, r) += (pi_0 * phi_tilde(w, r)
                        + pi_1 * ((n_wr(w, r) + beta_w) / denominator_phi));
      } else {
        e_phi(w, r) += ((n_wr(w, r) + beta_w) / denominator_phi);
        }
    }
  }
}

MatrixXd XName::getPhiHat(){
  return e_phi;
}



