import numpy as np
import pandas as pd
from sys import argv
from cvxopt import matrix, solvers

# ---------------------------
# Section: Load CSV and prepare arrays
# ---------------------------
file_path = argv[1]
output_params_file = argv[2]

data = pd.read_csv(file_path)

x = data['NumPages'].to_numpy().astype(float)
avg = data['Average'].to_numpy().astype(float)
mn = data['Min'].to_numpy().astype(float)

# ---------------------------
# Solve constrained least-squares (ensure line >= Min + margin for every point)
# Formulation:
#   minimize ||X z - y||^2  with z = [m, c]
#   subject to  m * x_i + c >= mn_i + margin   for all i
# This is a quadratic program (QP) with linear inequality constraints.
# ---------------------------
n = x.size
X = np.vstack([x, np.ones(n)]).T                 # design matrix for [m, c]
Q = 2.0 * (X.T @ X)                              # quadratic term (2 * X^T X)
p = -2.0 * (X.T @ avg)                           # linear term (-2 * X^T y)

# Add margin to the minimum constraint: m*x_i + c >= mn_i + margin
margin = (data['Average'] - data['Min']).min()
G = -X
h = -(mn + margin)

Q_cvx = matrix(Q)
p_cvx = matrix(p)
G_cvx = matrix(G)
h_cvx = matrix(h)

sol = solvers.qp(Q_cvx, p_cvx, G_cvx, h_cvx)
m = np.array(sol['x'])[0, 0]
c = np.array(sol['x'])[1, 0]

print(f"m = {m}")
print(f"c = {c}")
print(f"margin = {margin}")

with open(output_params_file, 'w') as F:
    F.write(f"slope,{m}\nintercept,{c}\nmargin,{margin}\n")
