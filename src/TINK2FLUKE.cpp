/*

##############################################################################
#                                                                            #
#              FLUKE: Fields Layered Under Kohn-sham Electrons               #
#                             By: Eric G. Kratz                              #
#                                                                            #
##############################################################################

 Functions to convert TINKER files to FLUKE format

 Reference for TINKER:
 Ponder, TINKER - Software Tools for Molecular Design

*/


//Parsers
void TINK2FLUKE(int& argc, char**& argv)
{
  //Local variables
  fstream TINKxyz,TINKkey,paramfile; //Input
  fstream posfile,confile,regfile; //Output
  stringstream line;
  string dummy; //Generic string
  bool TINKQMMM = 0;
  int Ninact = 0;
  bool SomeFroz = 0;
  bool SomeAct = 0;
  int ct; //Generic counter
  //Open files
  posfile.open("xyzfile.xyz",ios_base::out);
  confile.open("connect.inp",ios_base::out);
  regfile.open("regions.inp",ios_base::out);
  //Read arguments
  PBCon = 0;
  cout << '\n';
  cout << "Reading files: ";
  for (int i=0;i<argc;i++)
  {
    dummy = string(argv[i]);
    if (dummy == "-t")
    {
      TINKxyz.open(argv[i+1],ios_base::in);
      cout << argv[i+1];
      cout << " ";
    }
    if (dummy == "-k")
    {
      TINKkey.open(argv[i+1],ios_base::in);
      cout << argv[i+1];
      cout << " ";
    }
    if (dummy == "-p")
    {
      dummy = string(argv[i+1]);
      if ((dummy == "Yes") or (dummy == "yes") or
         (dummy == "YES") or (dummy == "true") or
         (dummy == "TRUE") or (dummy == "True"))
      {
        PBCon = 1;
      }
    }
  }
  if (PBCon == 1)
  {
    cout << '\n';
    cout << " with PBC";
  }
  cout << "...";
  cout << '\n';
  getline(TINKxyz,dummy);
  line.str(dummy);
  line >> Natoms;
  line >> Nqm;
  if (!line.eof())
  {
    line >> Npseudo;
    line >> Nbound;
  }
  else
  {
    Nqm = 0;
    Npseudo = 0;
    Nbound = 0;
  }
  Nmm = Natoms-Nqm-Npseudo-Nbound;
  if (Natoms != Nmm)
  {
    TINKQMMM = 1;
    cout << '\n';
    cout << "Auto-detected file type: ";
    cout << "TINKER QMMM xyz";
    cout << '\n';
  }
  if (Natoms == Nmm)
  {
    cout << '\n';
    cout << "Auto-detected file type: ";
    cout << "TINKER xyz";
    cout << '\n';
  }
  cout << "  Total atoms: " << Natoms << '\n';
  cout << "  QM atoms: " << Nqm << '\n';
  cout << "  Pseudo-atoms: " << Npseudo << '\n';
  cout << "  Boundary atoms: " << Nbound << '\n';
  cout << "  MM atoms: " << Nmm << '\n';
  cout << '\n';
  cout << "Creating xyzfile.xyz, connect.inp, and regions.inp";
  cout << "...";
  cout << '\n';
  //Create lists for grouping frozen atoms
  vector<bool> Actives;
  vector<bool> Froz;
  for (int i=0;i<Natoms;i++)
  {
    //Seems contradictory, however, this lets the script
    //check for both active and inactive commands
    Actives.push_back(0); //Define all as inactive
    Froz.push_back(0); //Define all as active
  }
  //Find parameter file, active atoms, and inactive atoms
  if (!TINKkey.good())
  {
    cout << '\n';
    cout << "Error: Could not read TINKER key file!";
    cout << '\n' << '\n';
    cout.flush();
    exit(0);
  }
  while (!TINKkey.eof())
  {
    //Read key file line by line
    getline(TINKkey,dummy);
    stringstream line(dummy);
    //Read string item by item
    line >> dummy;
    if (dummy == "parameters")
    {
      line >> dummy;
      paramfile.open(dummy.c_str(),ios_base::in);
    }
    if (dummy == "active")
    {
      SomeAct = 1;
      int AtNum;
      while (line >> AtNum)
      {
        Actives[AtNum-1] = 1;
      }
    }
    if (dummy == "inactive")
    {
      SomeFroz = 1;
      int AtNum;
      while (line >> AtNum)
      {
        Froz[AtNum-1] = 1;
      }
    }
  }
  if (!paramfile.good())
  {
    cout << '\n';
    cout << "Error: Could not open TINKER parameter file!";
    cout << '\n' << '\n';
    cout.flush();
    exit(0);
  }
  if ((SomeAct) or (SomeFroz))
  {
    for (int i=0;i<Natoms;i++)
    {
      if ((Actives[i] == 0) and (Froz[i] == 0))
      {
        if (SomeFroz)
        {
          Froz[i] = 0;
        }
        if (SomeAct)
        {
          Froz[i] = 1;
        }
      }
      if ((Actives[i] == 1) and (Froz[i] == 0))
      {
        Froz[i] = 0;
      }
      if ((Actives[i] == 0) and (Froz[i] == 1))
      {
        Froz[i] = 1;
      }
      if ((Actives[i] == 1) and (Froz[i] == 1))
      {
        Froz[i] = 0;
        cout << '\n';
        cout << "Warning: Atom " << i;
        cout << " is listed as both active and inactive.";
        cout << '\n';
        cout << " The atom will be assume to be active.";
        cout << '\n';
      }
    }
    for (int i=0;i<Natoms;i++)
    {
      //Count frozen atoms
      if (i >= (Nqm+Npseudo+Nbound))
      {
        if (Froz[i] == 1)
        {
          Ninact += 1;
        }
      }
      else
      {
        //QM, PA, and BA cannot be frozen
        Froz[i] = 0;
      }
    }
  }
  if (Ninact > 0)
  {
    cout << '\n';
    cout << "Detected " << Ninact;
    cout << " frozen atoms in the key file...";
    cout << '\n';
  }
  //Collect PBC information
  if (PBCon == 1)
  {
    //Grab whole line
    getline(TINKxyz,dummy);
    stringstream line(dummy);
    //Read box lengths
    line >> Lx >> Ly >> Lz;
  }
  //Write region file
  regfile << fixed;
  regfile << "PBC: ";
  if (PBCon == 1)
  {
    regfile << "Yes" << '\n';
    regfile << "Box_size: ";
    regfile << Lx << " ";
    regfile << Ly << " ";
    regfile << Lz << '\n';
  }
  else
  {
    regfile << "No" << '\n';
  }
  regfile << "QM_atoms: ";
  regfile << Nqm << '\n';
  ct = 0;
  for (int i=0;i<Nqm;i++)
  {
    regfile << i;
    ct += 1;
    if ((ct == 10) or (i == Nqm-1))
    {
      regfile << '\n';
      ct = 0;
    }
    else
    {
      regfile << " ";
    }
  }
  regfile << "Pseudo_atoms: ";
  regfile << Npseudo << '\n';
  ct = 0;
  for (int i=0;i<Npseudo;i++)
  {
    regfile << i+Nqm;
    ct += 1;
    if ((ct == 10) or (i == Npseudo-1))
    {
      regfile << '\n';
      ct = 0;
    }
    else
    {
      regfile << " ";
    }
  }
  regfile << "Boundary_atoms: ";
  regfile << Nbound << '\n';
  ct = 0;
  for (int i=0;i<Nbound;i++)
  {
    regfile << i+Nqm+Npseudo;
    ct += 1;
    if ((ct == 10) or (i == Nbound-1))
    {
      regfile << '\n';
      ct = 0;
    }
    else
    {
      regfile << " ";
    }
  }
  regfile << "Frozen_atoms: ";
  regfile << Ninact << '\n';
  ct = 0;
  for (int i=0;i<Natoms;i++)
  {
    if (Froz[i] == 1)
    {
      regfile << i;
      ct += 1;
      if (ct == 10)
      {
        regfile << '\n';
        ct = 0;
      }
      else
      {
        regfile << " ";
      }
    }
  }
  regfile << '\n';
  //Read force field parameters
  vector<vector<string> > Masses;
  vector<vector<string> > Ncharges;
  vector<vector<string> > Charges;
  while (!paramfile.eof())
  {
    //Read line by line for atom type info
    getline(paramfile,dummy);
    stringstream line(dummy);
    vector<string> fullline;
    while (line >> dummy)
    {
      fullline.push_back(dummy);
    }
    if (fullline.size() > 2)
    {
      if (fullline[0] == "atom")
      {
        vector<string> tmp;
        tmp.push_back(fullline[1]);
        tmp.push_back(fullline[fullline.size()-2]);
        Masses.push_back(tmp);
      }
      if (fullline[0] == "atom")
      {
        vector<string> tmp;
        tmp.push_back(fullline[1]);
        tmp.push_back(fullline[fullline.size()-3]);
        Ncharges.push_back(tmp);
      }
      if (fullline[0] == "charge")
      {
        vector<string> tmp;
        tmp.push_back(fullline[1]);
        tmp.push_back(fullline[2]);
        Charges.push_back(tmp);
      }
    }
  }
  //Create connectivity and xyx files
  posfile << Natoms << '\n' << '\n';
  for (int i=0;i<Natoms;i++)
  {
    getline(TINKxyz,dummy);
    stringstream line(dummy);
    vector<string> fullline;
    while (line >> dummy)
    {
      fullline.push_back(dummy);
    }
    string numtyp = fullline[5];
    bool Zfound = 0;
    for (int j=0;j<Ncharges.size();j++)
    {
      if (Ncharges[j][0] == numtyp)
      {
        //Find QM atom type
        stringstream line(Ncharges[j][1]);
        int NuChg;
        line >> NuChg;
        posfile << Typing(NuChg);
        posfile << " ";
        Zfound = 1;
      }
    }
    if (!Zfound)
    {
      //Print error
      cout << "Warning: Missing nuclear charge!";
      cout << " The .prm file may be incomplete.";
      cout << '\n';
      cout << "FLUKE cannot continue...";
      cout << '\n';
      //Dump files and exit
      posfile.flush();
      confile.flush();
      regfile.flush();
      cout.flush();
      exit(0);
    }
    //Write positions
    posfile << fullline[2] << " ";
    posfile << fullline[3] << " ";
    posfile << fullline[4] << '\n';
    //Write connectivity
    confile << i << " ";
    confile << fullline[1] << " ";
    confile << numtyp << " ";
    bool massfound = 0;
    for (int j=0;j<Masses.size();j++)
    {
      if (Masses[j][0] == numtyp)
      {
        confile << Masses[j][1] << " ";
        massfound = 1;
      }
    }
    if (!massfound)
    {
      //Print error
      cout << "Error: Missing mass! The .prm file may be incomplete.";
      cout << '\n';
      cout << "FLUKE cannot continue...";
      cout << '\n';
      //Dump output and quit
      posfile.flush();
      confile.flush();
      regfile.flush();
      cout.flush();
      exit(0);
    }
    bool chargefound = 0;
    for (int j=0;j<Charges.size();j++)
    {
      if (Charges[j][0] == numtyp)
      {
        confile << Charges[j][1] << " ";
        chargefound = 1;
      }
    }
    if (!chargefound)
    {
      //Print error
      cout << "Warning: Missing charge! The .prm file may be incomplete.";
      cout << '\n';
      confile << "0.00" << " "; //Set charge to zero
    }
    int nbonds = fullline.size()-6;
    confile << nbonds;
    for (int j=0;j<nbonds;j++)
    {
      confile << " ";
      int bondid;
      stringstream line(fullline[j+6]);
      line >> bondid;
      confile << (bondid-1);
    }
    confile << '\n';
  }
  //Quit FLUKE
  cout << '\n';
  cout << "Conversion complete.";
  cout << '\n';
  cout << '\n';
  cout.flush();
  TINKxyz.close();
  TINKkey.close();
  paramfile.close();
  posfile.close();
  confile.close();
  regfile.close();
  exit(0);
  return;
};

