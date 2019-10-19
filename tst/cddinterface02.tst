# CddInterface, single 2
#
# DO NOT EDIT THIS FILE - EDIT EXAMPLES IN THE SOURCE INSTEAD!
#
# This file has been generated by AutoDoc. It contains examples extracted from
# the package documentation. Each example is preceded by a comment which gives
# the name of a GAPDoc XML file and a line range from which the example were
# taken. Note that the XML file in turn may have been generated by AutoDoc
# from some other input.
#
gap> START_TEST( "cddinterface02.tst");

# doc/_Chunks.xml:65-86
gap> A:= Cdd_PolyhedronByGenerators( [ [ 0, 1, 3 ], [ 1, 4, 5 ] ] );
<Polyhedron given by its V-representation>
gap> Display( A );
V-representation
begin
   2 X 3  rational

   0  1  3
   1  4  5
end
gap> B:= Cdd_PolyhedronByGenerators( [ [ 0, 1, 3 ] ], [ 1 ] );
<Polyhedron given by its V-representation>
gap> Display( B );
V-representation
linearity 1, [ 1 ]
begin
   1 X 3  rational

   0  1  3
end

#
gap> STOP_TEST("cddinterface02.tst", 1 );
