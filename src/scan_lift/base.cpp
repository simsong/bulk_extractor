#include "myheader.h"
#include "base.h"

/* Document level functions */
// Document Level util functions

void Document::normalize_to_unit_length( ) {
  double sum = 0;
  EACH(it,Features) sum += it->second*it->second;
  sum = sqrt( sum );
  if( sum > EPS ) EACH(it,Features) it->second /= sum;
}

void Document::normalize_predicted_scores() {
  double sum = 0;
  EACH(it,label_scores) sum += it->second*it->second;
  sum = sqrt( sum );
  if( sum > EPS ) EACH(it,label_scores) it->second /= sum;
}

// Document Level print functions
void Document::print( FILE *f  ) const {
  /* Prints features with values zeros as well */
  REPV(i,labels) fprintf(f,"%s%d",i?",":"", labels[i]);
  fprintf(f," ");
  int maxx = -1;
  EACH(it,Features) assert( it->first > maxx ) , maxx = it->first;
  EACH(it,Features) if( fabs(it->second) > EPS ) fprintf(f,"%d:%lf ",it->first,it->second);
  fprintf(f,"# %d",docid);
}

#if 0
void Document::print_arff( FILE *f ) const { 
  /* The Weka ARFF file format */
  fprintf(f,"{");
  int maxx = -1, fcnt = MAX_LABELS;
  EACH(it,labels){
    if( maxx != -1 ) assert( *it > maxx );
    maxx = (*it);
  }
  maxx = -1;
  EACH(it,labels) fprintf( f , "%d %d,", (*it )-1 , 1 );
  EACH(it,Features) {
    if( maxx > -1 ) fprintf(f,",");
    assert( it->first > maxx ) , maxx = it->first;
    fprintf( f , "%d %lf", fcnt + it->first-1 , it->second );
  }
  fprintf(f , "}\n");
}
#endif

void Document::print_predicted( FILE *f ) const {
  assert( f );
  REPV(i,labels) fprintf(f,"%s%d",i?",":"",labels[i]);
  fprintf(f," %d",docid);
}

bool Document::parse_from_string( const char *str  ) {
  // Format = class1,class2,class3,.. feature1:value1 feature2:value2 ... # docid
    bool parsed_class = 0;
    //bool parsed_features = 0;
  int ptr = 0;
  int cls;
  // Move through non-numeric chars 
  while( str[ ptr ] && str[ ptr ] != ' ' ) {
    if( str[ ptr ] == ',' ) ptr++;
    sscanf( str + ptr , "%d" , &cls );
    labels.push_back( cls ), parsed_class = 1;
    while( str[ ptr ] && str[ ptr ] != ' ' && str[ ptr ] != ',' ) ptr++;
  }
  // Parse the Features
  
  while( str[ ptr ] && str[ ptr ] == ' ' && str[ ptr ] != '#' ) ptr++;
  for( ; str[ ptr ] && str[ ptr ] != '#' ;  ) {
    double v = -1;
    int f = -1;
    int r = sscanf( str + ptr , "%d:%lf", &f , &v );
    if( r != 2 ) { std::cerr << "scan_lift/base.cpp: PARSE ERROR =  "<< str << "\n"; return false; }
    while( str[ ptr ] && str[ ptr ] != ' ' && str[ ptr ] != '#' ) ptr++;
    while( str[ ptr ] && str[ ptr ] == ' ' && str[ ptr ] != '#' ) ptr++;
    if( f != -1 &&  ((int)v != -1 )){
	Features.push_back( make_pair( f , v ) );
	//parsed_features = true;
    }
  }
  
  if( str[ ptr ] == '#' ) sscanf( str + ptr+1 , "%d" , &docid );
  if( parsed_class == false ) std::cerr <<"scan_lift/base.cpp: PARSE ERROR = " << str << "\n";
  return parsed_class;
}

/* Bundle functions */

void DataBundle::normalize_to_unit_length( ) {  EACH(it,docs) it->normalize_to_unit_length(); }
void DataBundle::normalize_predicted_scores( ) { EACH(it,docs) it->normalize_predicted_scores(); }

void DataBundle::print( FILE *f ) const {
  EACH(it,docs) it->print( f ) , fprintf(f,"\n");
}

#if 0
void DataBundle::print_arff( FILE * f ) const { 
  assert( f );
  fprintf(f, "@relation MultiLabelData\n\n");
  int start = 1;
  EACH(it,labels) {
    while( start <= (*it) ) fprintf( f , "@attribute Class%d {0,1}\n", start++ );
  }
  while( start <= MAX_LABELS ) fprintf( f , "@attribute DummyAttribute%d {0,1}\n", start++ );
  REP(i,nfeatures) fprintf(f , "@attribute Att%d numeric\n", i );
  fprintf(f , "\n@data\n");
  EACH(it,docs) it->print_arff( f ); 
}
#endif

void DataBundle::print_predicted( FILE *f ) const {
  EACH(it,docs) it->print_predicted( f ) , fprintf(f,"\n");
}

void DataBundle::save_scores( const string& fname ) {
  FILE *f = fopen( fname.c_str() , "wb" );
  assert( f );
  EACH(it,(*this)) {
    sort( it->label_scores.begin() , it->label_scores.end() );    
    EACH(l,it->label_scores) fprintf(f , " %lf", l->second );
    fprintf(f , "\n");
  }
  fclose( f );
}

void DataBundle::save_dec( const string& fname ) {
  FILE *f = fopen( fname.c_str() , "wb" );
  assert( f );
  EACH(it,docs) {
    sort( it->predicted_labels.begin() , it->predicted_labels.end() ); 
    unsigned int a = 0;
    EACH(l,labels) { 
	if( a < it->predicted_labels.size() && it->predicted_labels[a] == (*l) ) 
	fprintf(f, " 1"), ++a;
      else
	fprintf(f, " 0");
    }
    fprintf(f,"\n");
  }
  fclose( f );
}

/* NOT USED WHILE PARSING FROM THE FILE */
void DataBundle::push_document( const Document& d ) {
  docs.push_back( d );
  if( d.labels.size() > 1 ) is_multilabel |= 1;
  // Sort the Features of the document after pushing it in , update the max no of features and also sort the labels
  sort( docs.back().Features.begin() , docs.back().Features.end() );
  sort( docs.back().labels.begin() , docs.back().labels.end() );
  if( docs.back().Features.size() ) nfeatures = max( nfeatures , docs.back().Features.back().first );
  // Update the number of labels, [ check for new labels ] 
  EACH(it,docs.back().labels) labels.insert( *it );
  EACH(it,docs.back().labels) label_counts[ *it ]++;
}


BaseClassifier::~BaseClassifier(){
}

