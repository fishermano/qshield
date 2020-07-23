// STL

#include <vector>
#include <algorithm>
#include "dag/DirectedAcyclicGraph.h"

#include "dag/Match.h"

// catch
#define CATCH_CONFIG_MAIN
#include "catch.hpp"


TEST_CASE("DAG Match"){
  typedef DAG::Node<const int> INode;

  // create a set of nodes
  INode n0(0);
  INode n1(1);
  INode n2(2);
  INode n3(3);
  INode n4(4);
  INode n5(5);
  INode n6(6);
  INode n7(7);
  INode n8(8);

  // and now define the dag
  n0.addChild(n1);  // link between n0 and n1
  n0.addChild(n2);  // link between n0 and n2 etc
  n0.addChild(n3);
  n1.addChild(n4);
  n1.addChild(n5);
  n1.addChild(n6);
  n7.addChild(n8);
  n7.addChild(n4);
  n3.addChild(n6);

  // Start at node 0 search for children
  // Breath First Search uses an iterative method to traverse
  // output the Datatype of all children
  std::cout << std::endl << "MATCH GIVEN PATTERN (start Node 0) " << std::endl;
  DAG::MatchVisitor<INode> mv;
  DAG::Edge<INode> t_edge_0(&n3, &n0, false);
  DAG::Edge<INode> t_edge_1(&n1, &n0, false);
  DAG::Edge<INode> t_edge_2(&n2, &n0, false);
  DAG::Edgeset<INode> t_edges = {&t_edge_0, &t_edge_1, &t_edge_2};

  DAG::Nodeset<INode> root = {&n0};

  bool hasMatch = mv.hasMatch(root, t_edges);
  REQUIRE(hasMatch == true);
}

TEST_CASE("DAG Exp 1: Q1"){
  typedef DAG::Node<const int> INode;

  clock_t start, end;

  /*
  // dag for Q1, m=5:
  //         4
  //       /
  //      3
  //    /
  //   0
  //    \
  //      1
  //        \
  //         2
  */
  INode n0(0);
  INode n1(1);
  INode n2(2);
  INode n3(3);
  INode n4(4);
  start = clock();
  n0.addChild(n1);
  n0.addChild(n3);
  n1.addChild(n2);
  n3.addChild(n4);
  end = clock();
  std::cout<<"DAG updating for Q1 totally spends: "<< ((double)(end -start)/CLOCKS_PER_SEC)*1000 << " milliseconds" << std::endl;

  DAG::Nodeset<INode> root = {&n0};

  // 3 DAG match required
  // 1
  DAG::MatchVisitor<INode> mv_1;
  DAG::Edge<INode> t_edge_1_0(&n2, &n1, false);
  DAG::Edgeset<INode> t_edge_1 = {&t_edge_1_0};
  // 2
  DAG::MatchVisitor<INode> mv_2;
  DAG::Edge<INode> t_edge_2_0(&n4, &n3, false);
  DAG::Edgeset<INode> t_edge_2 = {&t_edge_2_0};
  // 3
  DAG::MatchVisitor<INode> mv_3;
  DAG::Edge<INode> t_edge_3_0(&n3, &n0, false);
  DAG::Edge<INode> t_edge_3_1(&n1, &n0, false);
  DAG::Edgeset<INode> t_edge_3 = {&t_edge_3_0, &t_edge_3_1};

  start = clock();
  bool hasMatch_1 = mv_1.hasMatch(root, t_edge_1);
  bool hasMatch_2 = mv_2.hasMatch(root, t_edge_2);
  bool hasMatch_3 = mv_3.hasMatch(root, t_edge_3);
  end = clock();
  std::cout<<"DAG matching for Q1 totally spends: "<< ((double)(end -start)/CLOCKS_PER_SEC)*1000 << " milliseconds" << std::endl;
  REQUIRE(hasMatch_1 == true);
  REQUIRE(hasMatch_2 == true);
  REQUIRE(hasMatch_3 == true);
}

TEST_CASE("DAG Exp 2: Q2"){
  typedef DAG::Node<const int> INode;

  clock_t start, end;
  /*
  // dag for Q2, m=9:
  //               8
  //             /
  //            7
  //          /
  //         6
  //       /
  //      5
  //    /
  //   0
  //     \
  //      1
  //        \
  //         2
  //           \
  //            3
  //              \
  //               4
  */
  INode n0(0);
  INode n1(1);
  INode n2(2);
  INode n3(3);
  INode n4(4);
  INode n5(5);
  INode n6(6);
  INode n7(7);
  INode n8(8);
  start = clock();
  n0.addChild(n1);
  n0.addChild(n5);
  n1.addChild(n2);
  n2.addChild(n3);
  n3.addChild(n4);
  n5.addChild(n6);
  n6.addChild(n7);
  n7.addChild(n8);
  end = clock();
  std::cout<<"DAG updating for Q2 totally spends: "<< ((double)(end -start)/CLOCKS_PER_SEC)*1000 << " milliseconds" << std::endl;

  DAG::Nodeset<INode> root = {&n0};

  // 7 DAG match required
  // 1
  DAG::MatchVisitor<INode> mv_1;
  DAG::Edge<INode> t_edge_1_0(&n4, &n3, false);
  DAG::Edgeset<INode> t_edge_1 = {&t_edge_1_0};
  // 2
  DAG::MatchVisitor<INode> mv_2;
  DAG::Edge<INode> t_edge_2_0(&n3, &n2, false);
  DAG::Edgeset<INode> t_edge_2 = {&t_edge_2_0};
  // 3
  DAG::MatchVisitor<INode> mv_3;
  DAG::Edge<INode> t_edge_3_0(&n2, &n1, false);
  DAG::Edgeset<INode> t_edge_3 = {&t_edge_3_0};
  // 4
  DAG::MatchVisitor<INode> mv_4;
  DAG::Edge<INode> t_edge_4_0(&n1, &n0, false);
  DAG::Edge<INode> t_edge_4_1(&n5, &n0, false);
  DAG::Edgeset<INode> t_edge_4 = {&t_edge_4_0, &t_edge_4_1};
  // 5
  DAG::MatchVisitor<INode> mv_5;
  DAG::Edge<INode> t_edge_5_0(&n8, &n7, false);
  DAG::Edgeset<INode> t_edge_5 = {&t_edge_5_0};
  // 6
  DAG::MatchVisitor<INode> mv_6;
  DAG::Edge<INode> t_edge_6_0(&n7, &n6, false);
  DAG::Edgeset<INode> t_edge_6 = {&t_edge_6_0};
  // 7
  DAG::MatchVisitor<INode> mv_7;
  DAG::Edge<INode> t_edge_7_0(&n6, &n5, false);
  DAG::Edgeset<INode> t_edge_7 = {&t_edge_7_0};

  start = clock();
  bool hasMatch_1 = mv_1.hasMatch(root, t_edge_1);
  bool hasMatch_2 = mv_2.hasMatch(root, t_edge_2);
  bool hasMatch_3 = mv_3.hasMatch(root, t_edge_3);
  bool hasMatch_4 = mv_4.hasMatch(root, t_edge_4);
  bool hasMatch_5 = mv_5.hasMatch(root, t_edge_5);
  bool hasMatch_6 = mv_6.hasMatch(root, t_edge_6);
  bool hasMatch_7 = mv_7.hasMatch(root, t_edge_7);
  end = clock();
  std::cout<<"DAG matching for Q2 totally spends: "<< ((double)(end -start)/CLOCKS_PER_SEC)*1000 << " milliseconds" << std::endl;
  REQUIRE(hasMatch_7 == true);
  REQUIRE(hasMatch_6 == true);
  REQUIRE(hasMatch_5 == true);
  REQUIRE(hasMatch_4 == true);
  REQUIRE(hasMatch_3 == true);
  REQUIRE(hasMatch_2 == true);
  REQUIRE(hasMatch_1 == true);
}

TEST_CASE("DAG Exp 3: Q3"){
  typedef DAG::Node<const int> INode;

  clock_t start, end;
  /*
  // dag for Q3, m=19:
  //                      18
  //                     /
  //                   17
  //                  /
  //                16
  //               /
  //             13
  //            / \
  //          12   14
  //         /      \
  //       11        15
  //      /
  //     10
  //    /
  //   0
  //    \
  //     1
  //      \
  //       2       9
  //        \     /
  //         3   8
  //          \ /
  //           4
  //            \
  //             5
  //              \
  //               6
  //                \
  //                 7
  */
  INode n0(0);
  INode n1(1);
  INode n2(2);
  INode n3(3);
  INode n4(4);
  INode n5(5);
  INode n6(6);
  INode n7(7);
  INode n8(8);
  INode n9(9);
  INode n10(10);
  INode n11(11);
  INode n12(12);
  INode n13(13);
  INode n14(14);
  INode n15(15);
  INode n16(16);
  INode n17(17);
  INode n18(18);
  start = clock();
  n0.addChild(n1);
  n0.addChild(n10);
  n1.addChild(n2);
  n2.addChild(n3);
  n3.addChild(n4);
  n4.addChild(n5);
  n4.addChild(n8);
  n5.addChild(n6);
  n6.addChild(n7);
  n8.addChild(n9);
  n10.addChild(n11);
  n11.addChild(n12);
  n12.addChild(n13);
  n13.addChild(n14);
  n13.addChild(n16);
  n14.addChild(n15);
  n16.addChild(n17);
  n17.addChild(n18);
  end = clock();
  std::cout<<"DAG updating for Q3 totally spends: "<< ((double)(end -start)/CLOCKS_PER_SEC)*1000 << " milliseconds" << std::endl;

  DAG::Nodeset<INode> root = {&n0};

  // 16 DAG match required
  // 1
  DAG::MatchVisitor<INode> mv_1;
  DAG::Edge<INode> t_edge_1_0(&n7, &n6, false);
  DAG::Edgeset<INode> t_edge_1 = {&t_edge_1_0};
  // 2
  DAG::MatchVisitor<INode> mv_2;
  DAG::Edge<INode> t_edge_2_0(&n6, &n5, false);
  DAG::Edgeset<INode> t_edge_2 = {&t_edge_2_0};
  // 3
  DAG::MatchVisitor<INode> mv_3;
  DAG::Edge<INode> t_edge_3_0(&n9, &n8, false);
  DAG::Edgeset<INode> t_edge_3 = {&t_edge_3_0};
  // 4
  DAG::MatchVisitor<INode> mv_4;
  DAG::Edge<INode> t_edge_4_0(&n5, &n4, false);
  DAG::Edge<INode> t_edge_4_1(&n8, &n4, false);
  DAG::Edgeset<INode> t_edge_4 = {&t_edge_4_0, &t_edge_4_1};
  // 5
  DAG::MatchVisitor<INode> mv_5;
  DAG::Edge<INode> t_edge_5_0(&n4, &n3, false);
  DAG::Edgeset<INode> t_edge_5 = {&t_edge_5_0};
  // 6
  DAG::MatchVisitor<INode> mv_6;
  DAG::Edge<INode> t_edge_6_0(&n3, &n2, false);
  DAG::Edgeset<INode> t_edge_6 = {&t_edge_6_0};
  // 7
  DAG::MatchVisitor<INode> mv_7;
  DAG::Edge<INode> t_edge_7_0(&n2, &n1, false);
  DAG::Edgeset<INode> t_edge_7 = {&t_edge_7_0};
  // 8
  DAG::MatchVisitor<INode> mv_8;
  DAG::Edge<INode> t_edge_8_0(&n18, &n17, false);
  DAG::Edgeset<INode> t_edge_8 = {&t_edge_8_0};
  // 9
  DAG::MatchVisitor<INode> mv_9;
  DAG::Edge<INode> t_edge_9_0(&n17, &n16, false);
  DAG::Edgeset<INode> t_edge_9 = {&t_edge_2_0};
  // 10
  DAG::MatchVisitor<INode> mv_10;
  DAG::Edge<INode> t_edge_10_0(&n15, &n14, false);
  DAG::Edgeset<INode> t_edge_10 = {&t_edge_10_0};
  // 11
  DAG::MatchVisitor<INode> mv_11;
  DAG::Edge<INode> t_edge_11_0(&n14, &n13, false);
  DAG::Edge<INode> t_edge_11_1(&n16, &n13, false);
  DAG::Edgeset<INode> t_edge_11 = {&t_edge_11_0, &t_edge_11_1};
  // 12
  DAG::MatchVisitor<INode> mv_12;
  DAG::Edge<INode> t_edge_12_0(&n13, &n12, false);
  DAG::Edgeset<INode> t_edge_12 = {&t_edge_12_0};
  // 13
  DAG::MatchVisitor<INode> mv_13;
  DAG::Edge<INode> t_edge_13_0(&n12, &n11, false);
  DAG::Edgeset<INode> t_edge_13 = {&t_edge_13_0};
  // 14
  DAG::MatchVisitor<INode> mv_14;
  DAG::Edge<INode> t_edge_14_0(&n11, &n10, false);
  DAG::Edgeset<INode> t_edge_14 = {&t_edge_14_0};
  // 15
  DAG::MatchVisitor<INode> mv_15;
  DAG::Edge<INode> t_edge_15_0(&n1, &n0, false);
  DAG::Edge<INode> t_edge_15_1(&n10, &n0, false);
  DAG::Edgeset<INode> t_edge_15 = {&t_edge_15_0, &t_edge_15_1};

  start = clock();
  bool hasMatch_1 = mv_1.hasMatch(root, t_edge_1);
  bool hasMatch_2 = mv_2.hasMatch(root, t_edge_2);
  bool hasMatch_3 = mv_3.hasMatch(root, t_edge_3);
  bool hasMatch_4 = mv_4.hasMatch(root, t_edge_4);
  bool hasMatch_5 = mv_5.hasMatch(root, t_edge_5);
  bool hasMatch_6 = mv_6.hasMatch(root, t_edge_6);
  bool hasMatch_7 = mv_7.hasMatch(root, t_edge_7);
  bool hasMatch_8 = mv_8.hasMatch(root, t_edge_8);
  bool hasMatch_9 = mv_9.hasMatch(root, t_edge_9);
  bool hasMatch_10 = mv_10.hasMatch(root, t_edge_10);
  bool hasMatch_11 = mv_11.hasMatch(root, t_edge_11);
  bool hasMatch_12 = mv_12.hasMatch(root, t_edge_12);
  bool hasMatch_13 = mv_13.hasMatch(root, t_edge_13);
  bool hasMatch_14 = mv_14.hasMatch(root, t_edge_14);
  bool hasMatch_15 = mv_15.hasMatch(root, t_edge_15);
  end = clock();
  std::cout<<"DAG matching for Q3 totally spends: "<< ((double)(end -start)/CLOCKS_PER_SEC)*1000 << " milliseconds" << std::endl;
  REQUIRE(hasMatch_15 == true);
  REQUIRE(hasMatch_14 == true);
  REQUIRE(hasMatch_13 == true);
  REQUIRE(hasMatch_12 == true);
  REQUIRE(hasMatch_11 == true);
  REQUIRE(hasMatch_10 == true);
  REQUIRE(hasMatch_9 == true);
  REQUIRE(hasMatch_8 == true);
  REQUIRE(hasMatch_7 == true);
  REQUIRE(hasMatch_6 == true);
  REQUIRE(hasMatch_5 == true);
  REQUIRE(hasMatch_4 == true);
  REQUIRE(hasMatch_3 == true);
  REQUIRE(hasMatch_2 == true);
  REQUIRE(hasMatch_1 == true);
}
