/*
 * (C) Copyright 1996-2017 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */


#define BOOST_TEST_MODULE atlas_test_trans
#include "ecbuild/boost_test_framework.h"

#include <algorithm>

#include "atlas/atlas.h"
#include "atlas/field/FieldSet.h"
#include "atlas/functionspace/NodeColumns.h"
#include "atlas/functionspace/Spectral.h"
#include "atlas/functionspace/StructuredColumns.h"
#include "atlas/grid/GridDistribution.h"
#include "atlas/grid/grids.h"
#include "atlas/grid/partitioners/EqualRegionsPartitioner.h"
#include "atlas/grid/partitioners/TransPartitioner.h"
#include "atlas/mesh/generators/Structured.h"
#include "atlas/mesh/Mesh.h"
#include "atlas/mesh/Nodes.h"
#include "atlas/output/Gmsh.h"
#include "atlas/parallel/mpi/mpi.h"
#include "atlas/trans/Trans.h"
#include "transi/trans.h"

#include "tests/AtlasFixture.h"

using namespace eckit;

namespace atlas {
namespace test {

struct AtlasTransFixture : public AtlasFixture {
       AtlasTransFixture() {
         trans_init();
       }

      ~AtlasTransFixture() {
         trans_finalize();
       }
};


void read_rspecg(trans::Trans& trans, std::vector<double>& rspecg, std::vector<int>& nfrom, int &nfld )
{
  Log::info() << "read_rspecg ...\n";
  nfld = 2;
  if( trans.myproc(0) == 0 )
  {
    rspecg.resize(nfld*trans.nspec2g());
    for( int i=0; i<trans.nspec2g(); ++i )
    {
      rspecg[i*nfld + 0] = (i==0 ? 1. : 0.); // scalar field 1
      rspecg[i*nfld + 1] = (i==0 ? 2. : 0.); // scalar field 2
    }
  }
  nfrom.resize(nfld);
  for (int jfld=0; jfld<nfld; ++jfld)
    nfrom[jfld] = 1;

  Log::info() << "read_rspecg ... done" << std::endl;
}


BOOST_GLOBAL_FIXTURE( AtlasTransFixture );

BOOST_AUTO_TEST_CASE( test_trans_distribution_matches_atlas )
{
  BOOST_CHECK( grid::partitioners::PartitionerFactory::has("Trans") );


  // Create grid and trans object
  SharedPtr<grid::Structured> g ( grid::Structured::create( "N80" ) );

  BOOST_CHECK_EQUAL( g->nlat() , 160 );

  trans::Trans trans( *g );

  BOOST_CHECK_EQUAL( trans.nsmax() , 0 );

  grid::partitioners::TransPartitioner partitioner(*g,trans);
  grid::GridDistribution distribution( partitioner );

  // -------------- do checks -------------- //
  BOOST_CHECK_EQUAL( trans.nproc(),  parallel::mpi::comm().size() );
  BOOST_CHECK_EQUAL( trans.myproc(0), parallel::mpi::comm().rank() );


  if( parallel::mpi::comm().rank() == 0 ) // all tasks do the same, so only one needs to check
  {
    int max_nb_regions_EW(0);
    for( int j=0; j<partitioner.nb_bands(); ++j )
      max_nb_regions_EW = std::max(max_nb_regions_EW, partitioner.nb_regions(j));

    BOOST_CHECK_EQUAL( trans.n_regions_NS(), partitioner.nb_bands() );
    BOOST_CHECK_EQUAL( trans.n_regions_EW(), max_nb_regions_EW );

    BOOST_CHECK_EQUAL( distribution.nb_partitions(), parallel::mpi::comm().size() );
    BOOST_CHECK_EQUAL( distribution.partition().size(), g->npts() );

    std::vector<int> npts(distribution.nb_partitions(),0);

    for(size_t j = 0; j < g->npts(); ++j)
      ++npts[distribution.partition(j)];

    BOOST_CHECK_EQUAL( trans.ngptotg(), g->npts() );
    BOOST_CHECK_EQUAL( trans.ngptot(),  npts[parallel::mpi::comm().rank()] );
    BOOST_CHECK_EQUAL( trans.ngptotmx(), *std::max_element(npts.begin(),npts.end()) );

    array::ArrayView<int,1> n_regions ( trans.n_regions() ) ;
    for( int j=0; j<partitioner.nb_bands(); ++j )
      BOOST_CHECK_EQUAL( n_regions[j] , partitioner.nb_regions(j) );
  }
}


BOOST_AUTO_TEST_CASE( test_trans_partitioner )
{
  BOOST_TEST_CHECKPOINT("test_trans_partitioner");
  // Create grid and trans object
  SharedPtr<grid::Structured> g ( grid::Structured::create( "N80" ) );

  trans::Trans trans( *g, 0 );

  BOOST_CHECK_EQUAL( trans.nsmax() , 0 );
  BOOST_CHECK_EQUAL( trans.ngptotg() , g->npts() );
}

BOOST_AUTO_TEST_CASE( test_trans_options )
{
  trans::Trans::Options opts;
  opts.set_fft(trans::FFTW);
  opts.set_split_latitudes(false);
  opts.set_read("readfile");

  Log::info() << "trans_opts = " << opts << std::endl;
}


BOOST_AUTO_TEST_CASE( test_distspec )
{
  SharedPtr<grid::Structured> g ( grid::Structured::create( "O80" ) );
  mesh::generators::Structured generate( atlas::util::Config("angle",0) );
  BOOST_TEST_CHECKPOINT("mesh generator created");
  //trans::Trans trans(*g, 159 );

  trans::Trans::Options p;
  if( parallel::mpi::comm().size() == 1 )
    p.set_write("cached_legendre_coeffs");
  p.set_flt(false);
  trans::Trans trans(400, 159, p);
  BOOST_TEST_CHECKPOINT("Trans initialized");
  std::vector<double> rspecg;
  std::vector<int   > nfrom;
  int nfld;
  BOOST_TEST_CHECKPOINT("Read rspecg");
  read_rspecg(trans,rspecg,nfrom,nfld);


  std::vector<double> rspec(nfld*trans.nspec2());
  std::vector<int> nto(nfld,1);
  std::vector<double> rgp(nfld*trans.ngptot());
  std::vector<double> rgpg(nfld*trans.ngptotg());
  std::vector<double> specnorms(nfld,0);

  trans.distspec( nfld, nfrom.data(), rspecg.data(), rspec.data() );
  trans.specnorm( nfld, rspec.data(), specnorms.data() );
  trans.invtrans( nfld, rspec.data(), rgp.data() );
  trans.gathgrid( nfld, nto.data(),   rgp.data(),    rgpg.data() );

  if( parallel::mpi::comm().rank() == 0 ) {
    BOOST_CHECK_CLOSE( specnorms[0], 1., 1.e-10 );
    BOOST_CHECK_CLOSE( specnorms[1], 2., 1.e-10 );
  }

  BOOST_TEST_CHECKPOINT("end test_distspec");
}

BOOST_AUTO_TEST_CASE( test_distribution )
{
  SharedPtr<grid::Structured> g ( grid::Structured::create( "O80" ) );

  BOOST_TEST_CHECKPOINT("test_distribution");

  grid::GridDistribution::Ptr d_trans( grid::partitioners::TransPartitioner(*g).distribution() );
  BOOST_TEST_CHECKPOINT("trans distribution created");

  grid::GridDistribution::Ptr d_eqreg( grid::partitioners::EqualRegionsPartitioner(*g).distribution() );

  BOOST_TEST_CHECKPOINT("eqregions distribution created");

  if( parallel::mpi::comm().rank() == 0 )
  {
    BOOST_CHECK_EQUAL( d_trans->nb_partitions(), d_eqreg->nb_partitions() );
    BOOST_CHECK_EQUAL( d_trans->max_pts(), d_eqreg->max_pts() );
    BOOST_CHECK_EQUAL( d_trans->min_pts(), d_eqreg->min_pts() );

    BOOST_CHECK_EQUAL_COLLECTIONS( d_trans->nb_pts().begin(), d_trans->nb_pts().end(),
                                   d_eqreg->nb_pts().begin(), d_eqreg->nb_pts().end() );
  }

}


BOOST_AUTO_TEST_CASE( test_generate_mesh )
{
  BOOST_TEST_CHECKPOINT("test_generate_mesh");
  SharedPtr<grid::Structured> g ( grid::Structured::create( "O80" ) );
  mesh::generators::Structured generate( atlas::util::Config
    ("angle",0)
    ("triangulate",true)
  );
  trans::Trans trans(*g);

  mesh::Mesh::Ptr m_default( generate( *g ) );

  BOOST_TEST_CHECKPOINT("trans_distribution");
  grid::GridDistribution::Ptr trans_distribution( grid::partitioners::TransPartitioner(*g).distribution() );
  mesh::Mesh::Ptr m_trans( generate( *g, *trans_distribution ) );

  BOOST_TEST_CHECKPOINT("eqreg_distribution");
  grid::GridDistribution::Ptr eqreg_distribution( grid::partitioners::EqualRegionsPartitioner(*g).distribution() );
  mesh::Mesh::Ptr m_eqreg( generate( *g, *eqreg_distribution ) );

  array::ArrayView<int,1> p_default( m_default->nodes().partition() );
  array::ArrayView<int,1> p_trans  ( m_trans  ->nodes().partition() );
  array::ArrayView<int,1> p_eqreg  ( m_eqreg  ->nodes().partition() );

  BOOST_CHECK_EQUAL_COLLECTIONS( p_default.begin(), p_default.end(),
                                 p_trans  .begin(), p_trans  .end() );

  BOOST_CHECK_EQUAL_COLLECTIONS( p_default.begin(), p_default.end(),
                                 p_eqreg  .begin(), p_eqreg  .end() );

  //mesh::Mesh::Ptr mesh ( generate(*g, mesh::generators::EqualAreaPartitioner(*g).distribution() ) );

  output::Gmsh("N16_trans.msh").write(*m_trans);
}


BOOST_AUTO_TEST_CASE( test_spectral_fields )
{
  BOOST_TEST_CHECKPOINT("test_spectral_fields");

  SharedPtr<grid::Structured> g ( grid::Structured::create( "O48" ) );
  mesh::generators::Structured generate( atlas::util::Config
    ("angle",0)
    ("triangulate",false)
  );
  mesh::Mesh::Ptr m( generate( *g ) );

  trans::Trans trans(*g,47);


  SharedPtr<functionspace::NodeColumns> nodal (new functionspace::NodeColumns(*m));
  SharedPtr<functionspace::Spectral> spectral (new functionspace::Spectral(trans));

  SharedPtr<field::Field> spf ( spectral->createField<double>("spf") );
  SharedPtr<field::Field> gpf ( nodal->createField<double>("gpf") );


  BOOST_CHECK_NO_THROW( trans.dirtrans(*nodal,*gpf,*spectral,*spf) );
  BOOST_CHECK_NO_THROW( trans.invtrans(*spectral,*spf,*nodal,*gpf) );

  field::FieldSet gpfields;   gpfields.add(*gpf);
  field::FieldSet spfields;   spfields.add(*spf);

  BOOST_CHECK_NO_THROW( trans.dirtrans(*nodal,gpfields,*spectral,spfields) );
  BOOST_CHECK_NO_THROW( trans.invtrans(*spectral,spfields,*nodal,gpfields) );

  gpfields.add(*gpf);
  BOOST_CHECK_THROW(trans.dirtrans(*nodal,gpfields,*spectral,spfields),eckit::SeriousBug);

}


BOOST_AUTO_TEST_CASE( test_nomesh )
{
  BOOST_TEST_CHECKPOINT("test_spectral_fields");

  SharedPtr<grid::Structured> g ( grid::Structured::create( "O48" ) );
  SharedPtr<trans::Trans> trans ( new trans::Trans(*g,47) );

  SharedPtr<functionspace::Spectral>    spectral    (new functionspace::Spectral(*trans));
  SharedPtr<functionspace::StructuredColumns> gridpoints (new functionspace::StructuredColumns(*g));

  SharedPtr<field::Field> spfg ( spectral->createField<double>("spf",field::global()) );
  SharedPtr<field::Field> spf  ( spectral->createField<double>("spf") );
  SharedPtr<field::Field> gpf  ( gridpoints->createField<double>("gpf") );
  SharedPtr<field::Field> gpfg ( gridpoints->createField<double>("gpf", field::global()) );

  array::ArrayView<double,1> spg (*spfg);
  if( parallel::mpi::comm().rank() == 0 ) {
    spg = 0.;
    spg(0) = 4.;
  }

  BOOST_CHECK_NO_THROW( spectral->scatter(*spfg,*spf) );

  if( parallel::mpi::comm().rank() == 0 ) {
    array::ArrayView<double,1> sp (*spf);
    BOOST_CHECK_CLOSE( sp(0), 4., 0.001 );
    for( size_t jp=0; jp<sp.size(); ++jp ) {
      Log::debug() << "sp("<< jp << ")   :   " << sp(jp) << std::endl;
    }
  }

  BOOST_CHECK_NO_THROW( trans->invtrans(*spf,*gpf) );

  BOOST_CHECK_NO_THROW( gridpoints->gather(*gpf,*gpfg) );

  if( parallel::mpi::comm().rank() == 0 ) {
    array::ArrayView<double,1> gpg (*gpfg);
    for( size_t jp=0; jp<gpg.size(); ++jp ) {
      BOOST_CHECK_CLOSE( gpg(jp), 4., 0.001 );
      Log::debug() << "gpg("<<jp << ")   :   " << gpg(jp) << std::endl;
    }
  }

  BOOST_CHECK_NO_THROW( gridpoints->scatter(*gpfg,*gpf) );

  BOOST_CHECK_NO_THROW( trans->dirtrans(*gpf,*spf) );

  BOOST_CHECK_NO_THROW( spectral->gather(*spf,*spfg) );

  if( parallel::mpi::comm().rank() == 0 ) {
    BOOST_CHECK_CLOSE( spg(0), 4., 0.001 );
  }
}


} // namespace test
} // namespace atlas
