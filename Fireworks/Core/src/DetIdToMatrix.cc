#include <stdexcept>

#include "TGeoManager.h"
#include "TFile.h"
#include "TTree.h"
#include "TEveGeoNode.h"
#include <iostream>
#include <sstream>
#include "TPRegexp.h"
#include "TSystem.h"
#include "TGeoCompositeShape.h"
#include "TGeoBoolNode.h"

#include "Fireworks/Core/interface/DetIdToMatrix.h"
#include "Fireworks/Core/interface/fwLog.h"
#include "DataFormats/DetId/interface/DetId.h"

DetIdToMatrix::~DetIdToMatrix( void )
{
   // ATTN: not sure I own the manager
   // if ( manager_ ) delete manager_;
}

TFile*
DetIdToMatrix::findFile( const char* fileName )
{
   TString file;
   if( fileName[0] == '/' )
   {
      file= fileName;
   }
   else
   {
      if( const char* cmspath = gSystem->Getenv( "CMSSW_BASE" ))
      {
         file += cmspath;
         file += "/";
      }
      file += fileName;
   }
   if( !gSystem->AccessPathName( file.Data()))
   {
      return TFile::Open( file );
   }

   const char* searchpath = gSystem->Getenv( "CMSSW_SEARCH_PATH" );
   if( searchpath == 0 )
     return 0;
   TString paths( searchpath );
   TObjArray* tokens = paths.Tokenize( ":" );
   for( int i = 0; i < tokens->GetEntries(); ++i )
   {
      TObjString* path = (TObjString*)tokens->At( i );
      TString fullFileName( path->GetString());
      fullFileName += "/Fireworks/Geometry/data/";
      fullFileName += fileName;
      if( !gSystem->AccessPathName( fullFileName.Data()))
         return TFile::Open( fullFileName.Data());
   }
   return 0;
}

void
DetIdToMatrix::loadGeometry( const char* fileName )
{
   manager_ = 0;
   if( TFile* f = findFile( fileName ))
   {
      // load geometry
      manager_ = (TGeoManager*)f->Get( "cmsGeo" );
      f->Close();
   }
   else
   {
      throw std::runtime_error( "ERROR: failed to find geometry file. Initialization failed." );
   }
   if( !manager_ )
   {
      throw std::runtime_error( "ERROR: cannot find geometry in the file. Initialization failed." );
      return;
   }
}

void
DetIdToMatrix::loadMap( const char* fileName )
{
   if( !manager_ )
   {
      throw std::runtime_error( "ERROR: CMS detector geometry is not available. DetId to Matrix map Initialization failed." );
      return;
   }

   TFile* f = findFile( fileName );
   if( !f )
   {
      throw std::runtime_error( "ERROR: failed to find geometry file. Initialization failed." );
      return;
   }
   TTree* tree = (TTree*)f->Get( "idToGeo" );
   if( !tree )
   {
      throw std::runtime_error( "ERROR: cannot find detector id map in the file. Initialization failed." );
      return;
   }
   
   unsigned int id;
   char path[1000];
   Float_t points[24];
   Float_t topology[9];
   bool loadPoints = tree->GetBranch( "points" ) != 0;
   bool loadParameters = tree->GetBranch( "topology" ) != 0;
   tree->SetBranchAddress( "id", &id );
   tree->SetBranchAddress( "path", &path );
   if( loadPoints )
      tree->SetBranchAddress( "points", &points );
   if( loadParameters )
      tree->SetBranchAddress( "topology", &topology );
   
   for( unsigned int i = 0; i < tree->GetEntries(); ++i)
   {
      tree->GetEntry( i );
      idToPath_[id] = path;
      if( loadPoints )
      {
         std::vector<TEveVector> p(8);
         for( unsigned int j = 0; j < 8; ++j )
	    p[j].Set( points[3*j], points[3*j+1], points[3*j+2] );
         idToPoints_[id] = p;
      }
      if( loadParameters )
      {
         std::vector<Float_t> t(9);
         for( unsigned int j = 0; j < 9; ++j )
	    t[j] = topology[j];
         idToParameters_[id] = t;
      }      
   }
   f->Close();
}

const TGeoHMatrix*
DetIdToMatrix::getMatrix( unsigned int id ) const
{
   std::map<unsigned int, TGeoHMatrix>::const_iterator it = idToMatrix_.find( id );
   if( it != idToMatrix_.end()) return &( it->second );

   const char* path = getPath( id );
   if( !path )
      return 0;
   if( !manager_->cd(path))
   {
      DetId detId( id );
      fwLog( fwlog::kError ) << "incorrect path " << path << "\nfor DetId: " << detId.det() << " : " << id << std::endl;
      return 0;
   }

   TGeoHMatrix m = *( manager_->GetCurrentMatrix());

   idToMatrix_[id] = m;
   return &idToMatrix_[id];
}

const char*
DetIdToMatrix::getPath( unsigned int id ) const
{
   std::map<unsigned int, std::string>::const_iterator it = idToPath_.find( id );
   if( it != idToPath_.end())
      return it->second.c_str();
   else
      return 0;
}

const TGeoVolume*
DetIdToMatrix::getVolume( unsigned int id ) const
{
   std::map<unsigned int, std::string>::const_iterator it = idToPath_.find( id );
   if( it != idToPath_.end())
   {
      manager_->cd( it->second.c_str());
      return manager_->GetCurrentVolume();
   }
   else
      return 0;
}

std::vector<unsigned int>
DetIdToMatrix::getAllIds( void ) const
{
   std::vector<unsigned int> ids;
   for( std::map<unsigned int, std::string>::const_iterator it = idToPath_.begin(), itEnd = idToPath_.end();
	it != itEnd; ++it )
      ids.push_back( it->first );
   return ids;
}

std::vector<unsigned int>
DetIdToMatrix::getMatchedIds( const char* regular_expression ) const
{
   std::vector<unsigned int> ids;
   TPRegexp regexp( regular_expression );
   for( std::map<unsigned int, std::string>::const_iterator it = idToPath_.begin(), itEnd = idToPath_.end();
	it != itEnd; ++it )
      if( regexp.MatchB(it->second)) ids.push_back( it->first );
   return ids;
}


TEveGeoShape*
DetIdToMatrix::getShape( const char* path, const char* name, const TGeoMatrix* matrix /* = 0 */) const
{
   if( !manager_ || !path || !name )
      return 0;
   manager_->cd( path );
   // it's possible to get a corrected matrix from outside
   // if it's not provided, we take whatever the geo manager has
   if( !matrix )
      matrix = manager_->GetCurrentMatrix();

   TEveGeoShape* shape = new TEveGeoShape( name, path );
   shape->SetElementTitle( name );
   shape->SetTransMatrix( *matrix );
   TGeoShape* gs = manager_->GetCurrentVolume()->GetShape();
   //------------------------------------------------------------------------------//
   // FIXME !!!!!!!!!!!!!!
   // hack zone to make CSC complex shape visible
   // loop over bool shapes till we get something non-composite on the left side.
   if ( TGeoCompositeShape* composite = dynamic_cast<TGeoCompositeShape*>(gs) ){
     int depth = 0;
     TGeoShape* nextShape(gs);
     do {
       if ( depth > 10 ) break;
       nextShape = composite->GetBoolNode()->GetLeftShape();
       composite = dynamic_cast<TGeoCompositeShape*>(nextShape);
       ++depth;
     } while ( depth<10 && composite!=0 );
     if ( composite == 0 ) gs = nextShape;
   }
   //------------------------------------------------------------------------------//
   UInt_t id = TMath::Max(gs->GetUniqueID(), UInt_t(1));
   gs->SetUniqueID(id);
   shape->SetShape(gs);
   TGeoVolume* volume = manager_->GetCurrentVolume();
   shape->SetMainColor(volume->GetLineColor());
   shape->SetRnrSelf(kTRUE);
   shape->SetRnrChildren(kTRUE);
   return shape;
}

TEveGeoShape*
DetIdToMatrix::getShape( unsigned int id,
			 bool corrected /* = false */ ) const
{
   std::ostringstream s;
   s << id;
   if( corrected )
      return getShape( getPath(id), s.str().c_str(), getMatrix( id ));
   else
      return getShape( getPath(id), s.str().c_str());
}

TEveElementList*
DetIdToMatrix::getAllShapes( const char* elementListName /*= "CMS"*/) const
{
   TEveElementList* container = new TEveElementList( elementListName );
   for( std::map<unsigned int, std::string>::const_iterator it = idToPath_.begin(), itEnd = idToPath_.end();
	it != itEnd; ++it )
      container->AddElement( getShape( it->first ));
   return container;
}

std::vector<TEveVector>
DetIdToMatrix::getPoints( unsigned int id ) const
{
   // reco geometry points
   std::map<unsigned int, std::vector<TEveVector> >::const_iterator points = idToPoints_.find( id );
   if( points == idToPoints_.end())
   {
      fwLog(fwlog::kWarning) << "no reco geometry is found for id " <<  id << std::endl;
      return std::vector<TEveVector>();
   } else {
      return points->second;
   }
}

std::vector<Float_t>
DetIdToMatrix::getParameters( unsigned int id ) const
{
   // reco geometry parameters
   std::map<unsigned int, std::vector<Float_t> >::const_iterator parameters = idToParameters_.find( id );
   if( parameters == idToParameters_.end())
   {
      fwLog( fwlog::kWarning ) << "no reco parameters are found for id " <<  id << std::endl;
      return std::vector<Float_t>();
   }
   else
   {
      return parameters->second;
   }
}
