/*

##############################################################################
#                                                                            #
#              FLUKE: Fields Layered Under Kohn-sham Electrons               #
#                             By: Eric G. Kratz                              #
#                                                                            #
##############################################################################

 Path integral functions using QM, MM, and QMMM energies. Calls to wrappers
 are parallel over the number of beads. Other functions are mostly parallel
 over the number of atoms. It is safest to use serial wrappers for PIMC.

*/

//Path integral Monte Carlo functions
double SpringEnergy(double k, double r2)
{
  //General harmonic bond for PI rings
  double E = 0.5*k*r2;
  return E;
};

double Get_PI_Espring(vector<QMMMAtom>& parts, QMMMSettings& QMMMOpts)
{
  //Calculate total harmonic PI ring energy
  double E = 0.0;
  double w0 = 1/(QMMMOpts.Beta*hbar);
  w0 *= w0*ToeV;
  #pragma omp parallel for
  for (int i=0;i<Natoms;i++)
  {
    parts[i].Ep = 0.0;
    double w = w0*parts[i].m*QMMMOpts.Nbeads;
    for (int j=0;j<QMMMOpts.Nbeads;j++)
    {
      //Bead energy, one bond to avoid double counting
      int j2 = j-1;
      if (j2 == -1)
      {
        j2 = QMMMOpts.Nbeads-1; //Ring PBC
      }
      double dr2 = CoordDist2(parts[i].P[j],parts[i].P[j2]);
      parts[i].Ep += SpringEnergy(w,dr2);
    }
  }
  #pragma omp barrier
  for (int i=0;i<Natoms;i++)
  {
    E += parts[i].Ep;
  }
  return E;
};

double Get_PI_Epot(vector<QMMMAtom>& parts, QMMMSettings& QMMMOpts)
{
  //Potential for all beads
  double E = 0.0;
  vector<double> Es;
  vector<double> Times_qm;
  vector<double> Times_mm;
  for (int i=0;i<QMMMOpts.Nbeads;i++)
  {
    Es.push_back(0.0);
    Times_qm.push_back(0.0);
    Times_mm.push_back(0.0);
  }
  #pragma omp parallel for
  for (int i=0;i<QMMMOpts.Nbeads;i++)
  {
    //Timer variables
    int t_qm_start = 0;
    int t_mm_start = 0;
    //Runs the wrappers for all beads
    if (Gaussian == 1)
    {
      t_qm_start = (unsigned)time(0);
      Es[i] += GaussianWrapper("Enrg",parts,QMMMOpts,i);
      Times_qm[i] += (unsigned)time(0)-t_qm_start;
    }
    if (Psi4 == 1)
    {
      t_qm_start = (unsigned)time(0);
      Es[i] += PsiWrapper("Enrg",parts,QMMMOpts,i);
      Times_qm[i] += (unsigned)time(0)-t_qm_start;
      //Clean up annoying useless files
      int sys = system("rm -f psi.*");
    }
    if (Tinker == 1)
    {
      t_mm_start = (unsigned)time(0);
      Es[i] += TinkerWrapper("Enrg",parts,QMMMOpts,i);
      Times_mm[i] += (unsigned)time(0)-t_mm_start;
    }
    if (Amber == 1)
    {
      t_mm_start = (unsigned)time(0);
      Es[i] += AmberWrapper("Enrg",parts,QMMMOpts,i);
      Times_mm[i] += (unsigned)time(0)-t_mm_start;
    }
  }
  #pragma omp barrier
  for (int i=0;i<QMMMOpts.Nbeads;i++)
  {
    E += Es[i];
    QMTime += Times_qm[i];
    MMTime += Times_mm[i];
  }
  return E;
};

bool MCMove(vector<QMMMAtom>& parts, QMMMSettings& QMMMOpts)
{
  bool acc = 0;
  //Copy parts
  vector<QMMMAtom> parts2;
  parts2 = parts;
  //Pick random move and apply PBC
  double randnum = (((double)rand())/((double)RAND_MAX));
  if (randnum > (1-CentProb))
  {
    //Move a centroid
    int p;
    bool FrozenAt = 1;
    while (FrozenAt == 1)
    {
      //Make sure the atom is not frozen
      p = (rand()%Natoms);
      if (parts[p].Frozen != 1)
      {
        FrozenAt = 0;
      }
    }
    double randx = (((double)rand())/((double)RAND_MAX));
    double randy = (((double)rand())/((double)RAND_MAX));
    double randz = (((double)rand())/((double)RAND_MAX));
    double dx = 2*(randx-0.5)*step*Centratio;
    double dy = 2*(randy-0.5)*step*Centratio;
    double dz = 2*(randz-0.5)*step*Centratio;
    double x = parts2[p].x+dx;
    double y = parts2[p].y+dy;
    double z = parts2[p].z+dz;
    if (PBCon == 1)
    {
      bool check = 1;
      while (check == 1)
      {
        check = 0;
        if (x > Lx)
        {
          x -= Lx;
          check = 1;
        }
        if (x < 0.0)
        {
          x += Lx;
          check = 1;
        }
        if (y > Ly)
        {
          y -= Ly;
          check = 1;
        }
        if (y < 0.0)
        {
          y += Ly;
          check = 1;
        }
        if (z > Lz)
        {
          z -= Lz;
          check = 1;
        }
        if (z < 0.0)
        {
          z += Lz;
          check = 1;
        }
      }
    }
    parts2[p].x = x;
    parts2[p].y = y;
    parts2[p].z = z;
    #pragma omp parallel for
    for (int i=0;i<QMMMOpts.Nbeads;i++)
    {
      double xp = parts2[p].P[i].x+dx;
      double yp = parts2[p].P[i].y+dy;
      double zp = parts2[p].P[i].z+dz;
      if (PBCon == 1)
      {
        bool check = 1;
        while (check == 1)
        {
          check = 0;
          if (xp > Lx)
          {
            xp -= Lx;
            check = 1;
          }
          if (xp < 0.0)
          {
            xp += Lx;
            check = 1;
          }
          if (yp > Ly)
          {
            yp -= Ly;
            check = 1;
          }
          if (yp < 0.0)
          {
            yp += Ly;
            check = 1;
          }
          if (zp > Lz)
          {
            zp -= Lz;
            check = 1;
          }
          if (zp < 0.0)
          {
            zp += Lz;
            check = 1;
          }
        }
      }
      parts2[p].P[i].x = xp;
      parts2[p].P[i].y = yp;
      parts2[p].P[i].z = zp;
    }
    #pragma omp barrier
  }
  if (randnum < BeadProb)
  {
    //Move a single bead
    int p;
    bool FrozenAt = 1;
    while (FrozenAt == 1)
    {
      //Make sure the atom is not frozen
      p = (rand()%Natoms);
      if (parts[p].Frozen != 1)
      {
        FrozenAt = 0;
      }
    }
    int p2 = (rand()%QMMMOpts.Nbeads);
    double randx = (((double)rand())/((double)RAND_MAX));
    double randy = (((double)rand())/((double)RAND_MAX));
    double randz = (((double)rand())/((double)RAND_MAX));
    double dx = 2*(randx-0.5)*step;
    double dy = 2*(randy-0.5)*step;
    double dz = 2*(randz-0.5)*step;
    bool check = 1;
    parts2[p].P[p2].x += dx;
    if (PBCon == 1)
    {
      while (check == 1)
      {
        check = 0;
        if (parts2[p].P[p2].x > Lx)
        {
          parts2[p].P[p2].x -= Lx;
          check = 1;
        }
        if (parts2[p].P[p2].x < 0.0)
        {
          parts2[p].P[p2].x += Lx;
          check = 1;
        }
      }
    }
    parts2[p].P[p2].y += dy;
    if (PBCon == 1)
    {
      check = 1;
      while (check == 1)
      {
        check = 0;
        if (parts2[p].P[p2].y > Ly)
        {
          parts2[p].P[p2].y -= Ly;
          check = 1;
        }
        if (parts2[p].P[p2].y < 0.0)
        {
          parts2[p].P[p2].y += Ly;
          check = 1;
        }
      }
    }
    parts2[p].P[p2].z += dz;
    if (PBCon == 1)
    {
      check = 1;
      while (check == 1)
      {
        check = 0;
        if (parts2[p].P[p2].z > Lz)
        {
          parts2[p].P[p2].z -= Lz;
          check = 1;
        }
        if (parts2[p].P[p2].z < 0.0)
        {
          parts2[p].P[p2].z += Lz;
          check = 1;
        }
      }
    }
  }
  //Calculate energies
  double Eold = 0;
  Eold += Get_PI_Epot(parts,QMMMOpts);
  Eold += Get_PI_Espring(parts,QMMMOpts);
  double Enew = 0;
  double Lxtmp = Lx;
  double Lytmp = Ly;
  double Lztmp = Lz;
  randnum = (((double)rand())/((double)RAND_MAX));
  if (randnum < VolProb)
  {
    if (Isotrop == 0)
    {
      randnum = (((double)rand())/((double)RAND_MAX));
      double Lmin = 0.90*Lx;
      double Lmax = 1.10*Lx;
      Lx = Lmin+randnum*(Lmax-Lmin);
      randnum = (((double)rand())/((double)RAND_MAX));
      Lmin = 0.90*Ly;
      Lmax = 1.10*Ly;
      Ly = Lmin+randnum*(Lmax-Lmin);
      randnum = (((double)rand())/((double)RAND_MAX));
      Lmin = 0.90*Lz;
      Lmax = 1.10*Lz;
      Lz = Lmin+randnum*(Lmax-Lmin);
    }
    if (Isotrop == 1)
    {
      //Currently assumes that MM cutoffs are safe
      randnum = (((double)rand())/((double)RAND_MAX));
      double expan = 0.9+randnum*0.20;
      Lx *= expan;
      Ly *= expan;
      Lz *= expan;
    }
    #pragma omp parallel for
    for (int i=0;i<Natoms;i++)
    {
      for (int j=0;j<QMMMOpts.Nbeads;j++)
      {
        parts2[i].P[j].x *= Lx/Lxtmp;
        parts2[i].P[j].y *= Ly/Lytmp;
        parts2[i].P[j].z *= Lz/Lztmp;
      }
      parts2[i].x *= Lx/Lxtmp;
      parts2[i].y *= Ly/Lytmp;
      parts2[i].z *= Lz/Lztmp;
    }
    #pragma omp barrier
    Eold += QMMMOpts.Press*Lxtmp*Lytmp*Lztmp*atm2eV;
    Enew += QMMMOpts.Press*Lx*Ly*Lz*atm2eV;
  }
  Enew += Get_PI_Epot(parts2,QMMMOpts);
  Enew += Get_PI_Espring(parts2,QMMMOpts);
  //Accept or reject
  double dE = Enew-Eold;
  if (QMMMOpts.Ensemble == "NPT")
  {
    dE -= Natoms*log((Lx*Ly*Lz)/(Lxtmp*Lytmp*Lztmp))/QMMMOpts.Beta;
  }
  if ((dE == 0.0) and (Debug == 1))
  {
    cout << "Warning two structures have identical energies." << '\n';
  }
  double Prob = exp(-1*dE*QMMMOpts.Beta);
  randnum = (((double)rand())/((double)RAND_MAX));
  if ((dE <= 0) or (randnum < Prob))
  {
    parts = parts2;
    acc = 1;
  }
  else
  {
    Lx = Lxtmp;
    Ly = Lytmp;
    Lz = Lztmp;
  }
  return acc;
};

void Get_Centroid(QMMMAtom& part, QMMMSettings& QMMMOpts)
{
  //Finds the center of mass for the PI ring
  if (QMMMOpts.Nbeads != 0)
  {
    double x=0,y=0,z=0;
    for (int i=0;i<QMMMOpts.Nbeads;i++)
    {
      x += part.P[i].x;
      y += part.P[i].y;
      z += part.P[i].z;
    }
    x /= QMMMOpts.Nbeads;
    y /= QMMMOpts.Nbeads;
    z /= QMMMOpts.Nbeads;
    part.x = x;
    part.y = y;
    part.z = z;
    return;
  }
};

Coord Get_COM(vector<QMMMAtom>& parts, QMMMSettings& QMMMOpts)
{
  //Finds the center of mass for the entire system
  double x=0,y=0,z=0,M=0;
  Coord com;
  //Gather all atom centroids
  #pragma omp parallel for
  for (int i=0;i<Natoms;i++)
  {
    Get_Centroid(parts[i],QMMMOpts);
  }
  #pragma omp barrier
  //Calculate COM
  for (int i=0;i<Natoms;i++)
  {
    //Atoms
    x += parts[i].m*parts[i].x;
    y += parts[i].m*parts[i].y;
    z += parts[i].m*parts[i].z;
    M += parts[i].m;
  }
  com.x = x/M;
  com.y = y/M;
  com.z = z/M;
  return com;
};
