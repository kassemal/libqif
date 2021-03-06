#include "tests_aux.h"

// define a type-parametrized test case (https://code.google.com/p/googletest/wiki/AdvancedGuide)
template <typename T>
class ShannonTest : public BaseTest<T> {};

TYPED_TEST_CASE_P(ShannonTest);



TYPED_TEST_P(ShannonTest, Entropy) {
	typedef TypeParam eT;
	BaseTest<eT>& t = *this;

	EXPECT_PRED_FORMAT2(equal2<eT>, 1, shannon::entropy(t.unif_2));
	EXPECT_PRED_FORMAT2(equal2<eT>, qif::log2(10.0), shannon::entropy(t.unif_10));
	EXPECT_PRED_FORMAT2(equal2<eT>, 0, shannon::entropy(t.dirac_4));
	EXPECT_PRED_FORMAT2(equal2<eT>, 0.721928094887362, shannon::entropy(t.pi1));
}

TYPED_TEST_P(ShannonTest, Cond_entropy) {
	typedef TypeParam eT;
	BaseTest<eT>& t = *this;

	EXPECT_PRED_FORMAT2(equal2<eT>, 0, shannon::post_entropy(t.unif_2, t.id_2));
	EXPECT_PRED_FORMAT2(equal2<eT>, 0, shannon::post_entropy(t.dirac_2, t.id_2));
	EXPECT_PRED_FORMAT2(equal2<eT>, 0, shannon::post_entropy(t.pi1, t.id_2));

	EXPECT_PRED_FORMAT2(equal2<eT>, 0, shannon::post_entropy(t.unif_10, t.id_10));
	EXPECT_PRED_FORMAT2(equal2<eT>, 0, shannon::post_entropy(t.dirac_10, t.id_10));
	EXPECT_PRED_FORMAT2(equal2<eT>, 0, shannon::post_entropy(t.pi2, t.id_10));

	EXPECT_PRED_FORMAT2(equal2<eT>, qif::log2(10.0), shannon::post_entropy(t.unif_10, t.noint_10));
	EXPECT_PRED_FORMAT2(equal2<eT>, 0, shannon::post_entropy(t.dirac_10, t.noint_10));

	Prob<eT> pi = probab::randu<eT>(10);
	EXPECT_PRED_FORMAT2(equal2<eT>, shannon::entropy(pi), shannon::post_entropy(pi, t.noint_10));

	EXPECT_PRED_FORMAT2(equal2<eT>, 0.669020059980807, shannon::post_entropy(t.pi3, t.c1));

	ASSERT_ANY_THROW(shannon::post_entropy<eT>(t.unif_2, t.id_10););
}

TYPED_TEST_P(ShannonTest, Capacity) {
	typedef TypeParam eT;
	BaseTest<eT>& t = *this;

	Prob<eT> pi;

	EXPECT_PRED_FORMAT2(equal2<eT>, 1, shannon::add_capacity(t.id_2));
	EXPECT_PRED_FORMAT2(equal2<eT>, 1, shannon::add_capacity(t.id_2, pi));
	expect_prob(t.unif_2, pi);

	EXPECT_PRED_FORMAT2(equal2<eT>, qif::log2(10), shannon::add_capacity(t.id_10));
	EXPECT_PRED_FORMAT2(equal2<eT>, qif::log2(10), shannon::add_capacity(t.id_10, pi));
	expect_prob(t.unif_10, pi);

	eT md =	std::is_same<eT, float>::value ? 1e-6 : def_max_diff<eT>();		// the accuracy of 0 capacity is not great for floats
	EXPECT_PRED_FORMAT4(equal4<eT>, 0, shannon::add_capacity(t.noint_10), md, 0.0);

	EXPECT_PRED_FORMAT2(equal2<eT>, 0.19123813831431799, shannon::add_capacity(t.c1));

	// symmetric
	Chan<eT> C(
		".3 .2 .5;"
		".5 .3 .2;"
		".2 .5 .3;"
	);
	double cap = qif::log2(C.n_cols) - shannon::entropy<eT>(C.row(0));
	EXPECT_PRED_FORMAT2(equal2<eT>, cap, shannon::add_capacity(C));

	// weakly symmetric
	C = Chan<eT>(
		"0.333333333 0.166666667 0.5;"
		"0.333333333 0.5         0.166666667;"
	);
	cap = qif::log2(C.n_cols) - shannon::entropy<eT>(C.row(0));
	EXPECT_PRED_FORMAT2(equal2<eT>, cap, shannon::add_capacity(C));
}


// run the ChanTest test-case for double, float
//
REGISTER_TYPED_TEST_CASE_P(ShannonTest, Entropy, Cond_entropy, Capacity);

INSTANTIATE_TYPED_TEST_CASE_P(Shannon, ShannonTest, NativeTypes);

