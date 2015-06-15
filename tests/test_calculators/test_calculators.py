import os
import sys
import math

# Fisrt, we add the location of the library to test to the PYTHON path
cwd = os.getcwd()
print "Current working directory", cwd
sys.path.insert(1,cwd+"/../../_build/src/mmath")
sys.path.insert(1,cwd+"/../../_build/src/calculators")


print "\nTest 1: Importing the library and its content"
from cygmmath import *
from cygcalculators import *


print "\n Test2: Setting up matrix"
e = [-1.0, -0.5, -0.4]
a = MATRIX(3,3)
for i in range(0,3):
    for j in range(0,3):
        if i==j:
            a.set(i,j, e[i])
        else:
            a.set(i,j, 0.0)

a.show_matrix()

print "\n Test3: Fermi energy"
Nel = 2.0
degen = 1.0
kT = 0.025
etol = 0.0001
Ef = fermi_energy(e, Nel, degen, kT, etol)

print "Fermi energy = ", Ef


print "\n Test4: order bands"
bnds = order_bands(a)
print bnds


print "\n Test5: populate bands"
Nocc = Nel
pop_opt = 0
occ = populate_bands(Nel, degen, kT, etol, pop_opt, bnds)
print occ

pop_opt = 1
occ = populate_bands(Nel, degen, kT, etol, pop_opt, bnds)
print occ


print "\n Test6: Test Fock to P function:"
f = [-1.0, -0.5, -0.4]
F = MATRIX(3,3)
S = MATRIX(3,3)
for i in range(0,3):
    for j in range(0,3):
        if i==j:
            F.set(i,j, e[i])
            S.set(i,j, 1.0)
        else:
            F.set(i,j, 0.0)
            S.set(i,j, 0.0)

F.show_matrix()
S.show_matrix()

pop_opt = 0
res = Fock_to_P(F,S, Nel, degen, kT, etol, pop_opt)

print "Eigenvalues:\n"
res[0].show_matrix()

print "Eigenvectors:\n"
res[1].show_matrix()

print "Density matrix:\n"
res[2].show_matrix()

print "Bands:\n"
print res[3]

print "Occupations:\n"
print res[4]


print "\n Test7: Excitations:"
ex1 = excite(1, 2, res[4]);
ex2 = excite(0, 2, res[4]);

print "ex1 = ", ex1
print "ex2 = ", ex2


print "\n Test8: Energies and density matrix"
print "energy(GS) = ", energy_elec(res[2],F,F)

P_ex1 = compute_density_matrix(ex1,res[1])
print "energy(Ex1) = ", energy_elec(P_ex1,F,F)

P_ex2 = compute_density_matrix(ex2,res[1])
print "energy(Ex2) = ", energy_elec(P_ex2,F,F)




