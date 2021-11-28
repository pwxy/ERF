#include "prob.H"

ProbParm parms;

void
erf_init_dens_hse(amrex::Real* dens_hse_ptr,
                  amrex::GeometryData const& geomdata,
                  const int ng_dens_hse)
{
  const int khi = geomdata.Domain().bigEnd()[2];
  for (int k = -ng_dens_hse; k <= khi+ng_dens_hse; k++)
  {
      dens_hse_ptr[k] = parms.rho_0;
  }
}

void
erf_init_prob(
  const amrex::Box& bx,
  amrex::Array4<amrex::Real> const& state,
  amrex::Array4<amrex::Real> const& x_vel,
  amrex::Array4<amrex::Real> const& y_vel,
  amrex::Array4<amrex::Real> const& z_vel,
  amrex::GeometryData const& geomdata)
{
  amrex::ParallelFor(bx, [=, parms=parms] AMREX_GPU_DEVICE(int i, int j, int k) noexcept {
    // Geometry
    const amrex::Real* prob_lo = geomdata.ProbLo();
    const amrex::Real* prob_hi = geomdata.ProbHi();
    const amrex::Real* dx = geomdata.CellSize();
    const amrex::Real x = prob_lo[0] + (i + 0.5) * dx[0];
    const amrex::Real y = prob_lo[1] + (j + 0.5) * dx[1];
    const amrex::Real z = prob_lo[2] + (k + 0.5) * dx[2];

    // Define a point (xc,yc,zc) at the center of the domain
    const amrex::Real xc = 0.5 * (prob_lo[0] + prob_hi[0]);
    const amrex::Real yc = 0.5 * (prob_lo[1] + prob_hi[1]);
    const amrex::Real zc = 0.5 * (prob_lo[2] + prob_hi[2]);

    const amrex::Real r  = std::sqrt((x-xc)*(x-xc) + (y-yc)*(y-yc) + (z-zc)*(z-zc));

    // Set the density
    state(i, j, k, Rho_comp) = parms.rho_0;

    // Initial Rho0*Theta0
    state(i, j, k, RhoTheta_comp) = parms.rho_0 * parms.T_0;

    // Set scalar = A_0*exp(-10r^2), where r is distance from center of domain
    state(i, j, k, RhoScalar_comp) = parms.A_0 * exp(-10.*r*r);
  });

  const amrex::Box& xbx = amrex::surroundingNodes(bx,0);
  amrex::ParallelForRNG(xbx, [=, parms=parms] AMREX_GPU_DEVICE(int i, int j, int k, const amrex::RandomEngine& engine) noexcept {

    amrex::Real rand_double = amrex::Random(engine); // Between 0.0 and 1.0
    amrex::Real x_vel_prime = (rand_double*2.0 - 1.0)*parms.U0_Pert_Mag;
    x_vel(i, j, k) = parms.U0 + x_vel_prime;
  });

  const amrex::Box& ybx = amrex::surroundingNodes(bx,1);
  amrex::ParallelForRNG(ybx, [=, parms=parms] AMREX_GPU_DEVICE(int i, int j, int k, const amrex::RandomEngine& engine) noexcept {

    amrex::Real rand_double = amrex::Random(engine); // Between 0.0 and 1.0
    amrex::Real y_vel_prime = (rand_double*2.0 - 1.0)*parms.V0_Pert_Mag;
    y_vel(i, j, k) = parms.V0 + y_vel_prime;
  });

  const amrex::Box& zbx = amrex::surroundingNodes(bx,2);
  amrex::ParallelForRNG(zbx, [=, parms=parms] AMREX_GPU_DEVICE(int i, int j, int k, const amrex::RandomEngine& engine) noexcept {

    // Set the z-velocity
    amrex::Real rand_double = amrex::Random(engine); // Between 0.0 and 1.0
    amrex::Real z_vel_prime = (rand_double*2.0 - 1.0)*parms.W0_Pert_Mag;
    z_vel(i, j, k) = parms.W0 + z_vel_prime;
  });
}

void
erf_prob_close()
{
}

extern "C" {
void
amrex_probinit(
  const int* init,
  const int* name,
  const int* namelen,
  const amrex_real* problo,
  const amrex_real* probhi)
{
  // Parse params
  amrex::ParmParse pp("prob");
  pp.query("rho_0", parms.rho_0);
  pp.query("T_0", parms.T_0);
  pp.query("A_0", parms.A_0);

  pp.query("U0", parms.U0);
  pp.query("V0", parms.V0);
  pp.query("W0", parms.W0);
  pp.query("U0_Pert_Mag", parms.U0_Pert_Mag);
  pp.query("V0_Pert_Mag", parms.V0_Pert_Mag);
  pp.query("W0_Pert_Mag", parms.W0_Pert_Mag);
}
}
