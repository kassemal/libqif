
namespace mechanism {

// Returns the mechanism satisfying eps*d privacy and having the best utility wrt pi and loss
//
template<typename eT>
Mech<eT> optimal_utility(Prob<eT> pi, uint n_cols, Metric<eT, uint> d_priv, Metric<eT, uint> loss, eT epsilon = eT(1)) {
	// C: M x N   unknowns
	// We have M x N variables, that will be unfolded in a vector.
	// The varialbe C[x,y] will have variable number xN+y.
	//
	uint M = pi.n_cols,
		 N = n_cols,
		 n_vars = M * N,					// one var for each element of C
		 n_cons = M*(M-1)*N+M,				// one constraint for each C_xy, C_x'y, plus M sum=1 constraints
		 n_cons_elems = 2*M*(M-1)*N+M*N;	// 2 elems for each DP constraint + M*N elements for the sum=1 constraints

	LinearProgram<eT> lp;
	lp.b.set_size(n_cons);
	lp.sense.set_size(n_cons);

	// cost function: minimize sum_xy pi_x C_xy loss(x,y)
	lp.maximize = false;
	lp.c = Col<eT>(n_vars);
	for(uint x = 0; x < M; x++)
		for(uint y = 0; y < N; y++)
			lp.c(x*N+y) = pi(x) * loss(x, y);

	arma::umat locations(2, n_cons_elems);	// for batch-insertion into sparse matrix lp.A
	Col<eT> values(n_cons_elems);
	uint elem_i = 0, cons_i = 0;

	// Build equations for C_xy <= exp(eps d_priv(x,x')) C_x'y
	//
	for(uint x1 = 0; x1 < M; x1++) {
	for(uint x2 = 0; x2 < M; x2++) {
		if(x1 == x2) continue;
		for(uint y = 0; y < N; y++) {

			lp.sense(cons_i) = '<';
			lp.b(cons_i) = eT(0);

			locations(0, elem_i) = cons_i;
			locations(1, elem_i) = x1*N+y;
			values(elem_i) = eT(1);
			elem_i++;

			locations(0, elem_i) = cons_i;
			locations(1, elem_i) = x2*N+y;
			values(elem_i) = - std::exp(epsilon * d_priv(x1, x2));
			elem_i++;

			cons_i++;
		}
	}}

	// equalities for summing up to 1
	//
	for(uint x = 0; x < M; x++) {
		lp.b(cons_i) = eT(1);					// sum of row = 1
		lp.sense(cons_i) = '=';

		for(uint y = 0; y < N; y++) {
			locations(0, elem_i) = cons_i;
			locations(1, elem_i) = x*N+y;
			values(elem_i) = eT(1);				// coeff 1 for variable C[x,y]
			elem_i++;
		}
		cons_i++;
	}

	assert(cons_i == n_cons);									// added all constraints
	assert(elem_i == n_cons_elems);								// added all constraint elements

	lp.A = arma::SpMat<eT>(locations, values, n_cons, n_vars);	// arma has no batch-insert method into existing lp.A

	Mech<eT> mech;
	mech.d = d_priv;

	// solve program
	//
	if(!lp.solve())
		return mech;

	// reconstrict channel from solution
	//
	mech.C.set_size(M, N);
	for(uint x = 0; x < M; x++)
		for(uint y = 0; y < N; y++)
			mech.C(x, y) = lp.x(x*N+y);

	return mech;
}

template<typename eT>
Mech<eT> dist_optimal_utility(Prob<eT> pi, uint n_cols, Metric<eT, uint> d_priv, Metric<eT, uint> loss, eT epsilon = eT(1)) {

	uint M = pi.n_cols,
		 N = n_cols;

	// insert all distances in a std::set to keep unique ones.
	// Then collect in dists vector and sort
	//
	std::set<eT> dists_set;
	for(uint x = 0; x < M; x++)
		for(uint y = 0; y < N; y++)
			dists_set.insert(d_priv(x, y));

	uint D = dists_set.size();
	Col<eT> dists(D);
	uint i = 0;
	for(eT v : dists_set)
		dists(i++) = v;
	dists = arma::sort(dists);

	// DI : MxN matrix, DIxy is the index of d_priv(x,y) in dists
	Mat<uint> DI(M, N);
	for(uint x = 0; x < M; x++)
		for(uint y = 0; y < N; y++)
			DI(x, y) = arma::ucolvec(arma::find(dists == d_priv(x, y), 1)).at(0);

	// variable X[d,y] will have index d*N+y
	//
	uint n_vars = D*N,						// one var for each distinct distance and output
		 n_cons = 2*(D-1)*N+M,				// 2 constraints for each consecutive distances and output, plus M sum=1 constraints
		 n_cons_elems = 4*(D-1)*N+M*N;		// 2 elems for each DP constraint + M*N elements for the sum=1 constraints

	LinearProgram<eT> lp;
	lp.b.set_size(n_cons);
	lp.sense.set_size(n_cons);

	// cost function: minimize sum_xy pi_x X[d(x,y),y] loss(x,y)
	lp.maximize = false;
	lp.c = Col<eT>(n_vars, arma::fill::zeros);
	for(uint x = 0; x < M; x++)
		for(uint y = 0; y < N; y++)
			lp.c(DI(x,y)*N+y) += pi(x) * loss(x, y);

	arma::umat locations(2, n_cons_elems);	// for batch-insertion into sparse matrix lp.A
	Col<eT> values(n_cons_elems);
	uint elem_i = 0, cons_i = 0;

	// Build equations for X_d_i <= exp(eps |d_i - d_i+1|) X_d_j
	//
	for(uint dist_i = 0; dist_i < D-1; dist_i++) {
		eT diff = dists(dist_i+1) - dists(dist_i);

		for(uint y = 0; y < N; y++) {
			lp.sense(cons_i) = '<';
			lp.b(cons_i) = eT(0);

			locations(0, elem_i) = cons_i;
			locations(1, elem_i) = dist_i*N+y;
			values(elem_i) = eT(1);
			elem_i++;

			locations(0, elem_i) = cons_i;
			locations(1, elem_i) = (dist_i+1)*N+y;
			values(elem_i) = - std::exp(epsilon * diff);
			elem_i++;

			cons_i++;

			lp.sense(cons_i) = '<';
			lp.b(cons_i) = eT(0);

			locations(0, elem_i) = cons_i;
			locations(1, elem_i) = (dist_i+1)*N+y;
			values(elem_i) = eT(1);
			elem_i++;

			locations(0, elem_i) = cons_i;
			locations(1, elem_i) = dist_i*N+y;
			values(elem_i) = - std::exp(epsilon * diff);
			elem_i++;

			cons_i++;
		}
	}

	// equalities for summing up to 1
	// Note: if the same distance appears multiple times in the same row, we're going to insert multiple 1's
	// for the same cell, which are summed due to the "true" param in the batch-insert below
	//
	for(uint x = 0; x < M; x++) {
		lp.b(cons_i) = eT(1);					// sum of row = 1
		lp.sense(cons_i) = '=';

		for(uint y = 0; y < N; y++) {
			locations(0, elem_i) = cons_i;
			locations(1, elem_i) = DI(x,y)*N+y;
			values(elem_i) = eT(1);
			elem_i++;
		}
		cons_i++;
	}

	assert(cons_i == n_cons);									// added all constraints
	assert(elem_i == n_cons_elems);								// added all constraint elements

	lp.A = arma::SpMat<eT>(true, locations, values, n_cons, n_vars);	// arma has no batch-insert method into existing lp.A

	Mech<eT> mech;
	mech.d = d_priv;

	// solve program
	//
	if(!lp.solve())
		return mech;

	// reconstrict channel from solution
	//
	mech.C.set_size(M, N);
	for(uint x = 0; x < M; x++)
		for(uint y = 0; y < N; y++)
			mech.C(x, y) = lp.x(DI(x,y)*N+y);

	return mech;
}

// This is the first version, that had variables X_d instead of X_d,y
//
template<typename eT>
Mech<eT> dist_optimal_utility_strict(Prob<eT> pi, uint n_cols, Metric<eT, uint> d_priv, Metric<eT, uint> loss, eT epsilon = eT(1)) {

	uint M = pi.n_cols,
		 N = n_cols;

	// insert all distances in a std::set to keep unique ones.
	// Then collect in dists vector and sort
	//
	std::set<eT> dists_set;
	for(uint x = 0; x < M; x++)
		for(uint y = 0; y < N; y++)
			dists_set.insert(d_priv(x, y));

	uint D = dists_set.size();
	Col<eT> dists(D);
	uint i = 0;
	for(eT v : dists_set)
		dists(i++) = v;
	dists = arma::sort(dists);

	// DI : MxN matrix, DIxy is the index of d_priv(x,y) in dists
	Mat<uint> DI(M, N);
	for(uint x = 0; x < M; x++)
		for(uint y = 0; y < N; y++)
			DI(x, y) = arma::ucolvec(arma::find(dists == d_priv(x, y), 1)).at(0);

	// C: D variables
	//
	uint n_vars = D,						// one var for each distinct distance
		 n_cons = 2*(D-1)+M,				// 2 constraints for each consecutive distances, plus M sum=1 constraints
		 n_cons_elems = 4*(D-1)+M*N;		// 2 elems for each DP constraint + M*N elements for the sum=1 constraints

	LinearProgram<eT> lp;
	lp.b.set_size(n_cons);
	lp.sense.set_size(n_cons);

	// cost function: minimize sum_xy pi_x var<C_xy> loss(x,y)
	lp.maximize = false;
	lp.c = Col<eT>(n_vars, arma::fill::zeros);
	for(uint x = 0; x < M; x++)
		for(uint y = 0; y < N; y++)
			lp.c(DI(x,y)) += pi(x) * loss(x, y);

	arma::umat locations(2, n_cons_elems);	// for batch-insertion into sparse matrix lp.A
	Col<eT> values(n_cons_elems);
	uint elem_i = 0, cons_i = 0;

	// Build equations for X_d_i <= exp(eps |d_i - d_i+1|) X_d_j
	//
	for(uint dist_i = 0; dist_i < D-1; dist_i++) {
		eT diff = dists(dist_i+1) - dists(dist_i);

		lp.sense(cons_i) = '<';
		lp.b(cons_i) = eT(0);

		locations(0, elem_i) = cons_i;
		locations(1, elem_i) = dist_i;
		values(elem_i) = eT(1);
		elem_i++;

		locations(0, elem_i) = cons_i;
		locations(1, elem_i) = dist_i+1;
		values(elem_i) = - std::exp(epsilon * diff);
		elem_i++;

		cons_i++;

		lp.sense(cons_i) = '<';
		lp.b(cons_i) = eT(0);

		locations(0, elem_i) = cons_i;
		locations(1, elem_i) = dist_i+1;
		values(elem_i) = eT(1);
		elem_i++;

		locations(0, elem_i) = cons_i;
		locations(1, elem_i) = dist_i;
		values(elem_i) = - std::exp(epsilon * diff);
		elem_i++;

		cons_i++;
	}

	// equalities for summing up to 1
	// Note: if the same distance appears multiple times in the same row, we're going to insert multiple 1's
	// for the same cell, which are summed due to the "true" param in the batch-insert below
	//
	for(uint x = 0; x < M; x++) {
		lp.b(cons_i) = eT(1);					// sum of row = 1
		lp.sense(cons_i) = '=';

		for(uint y = 0; y < N; y++) {
			locations(0, elem_i) = cons_i;
			locations(1, elem_i) = DI(x,y);
			values(elem_i) = eT(1);
			elem_i++;
		}
		cons_i++;
	}

	assert(cons_i == n_cons);									// added all constraints
	assert(elem_i == n_cons_elems);								// added all constraint elements

	lp.A = arma::SpMat<eT>(true, locations, values, n_cons, n_vars);	// arma has no batch-insert method into existing lp.A

	Mech<eT> mech;
	mech.d = d_priv;

	// solve program
	//
	if(!lp.solve())
		return mech;

	// reconstrict channel from solution
	//
	mech.C.set_size(M, N);
	for(uint x = 0; x < M; x++)
		for(uint y = 0; y < N; y++)
			mech.C(x, y) = lp.x(DI(x,y));

	return mech;
}

} // namespace mechanism