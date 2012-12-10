#include "base.h"

class IRDocument : public Document { 
  /* qid should be positive and relevant should be either -1, +1 or 0.*/
public:
    IRDocument():qid(-1),relevant(0),score(0){}

    int qid, relevant ;
    double score;
  
    void print( FILE *f ) const ;
};

struct IRDataBundle {
  int nfeatures , is_training;
  string dataset_name;
  vector< IRDocument > docs;

  IRDataBundle( bool is_training  , string dataset_name );

  void push_document( const IRDocument& d );
  string get_dataset_name() const                    { return dataset_name; }
  void set_dataset_name( const string& name)         { dataset_name = name; }
  int get_nfeatures() const                          { return nfeatures; } 
  int size() const                                   { return docs.size(); }
  

  typedef std::vector< IRDocument >::iterator iterator;
  typedef std::vector< IRDocument >::const_iterator const_iterator;
  iterator begin()                                     { return docs.begin(); }
  const_iterator begin() const                         { return docs.begin(); }
  iterator end()                                       { return docs.end(); }
  const_iterator end() const                           { return docs.end(); }


};

void IRDocument::print( FILE *f ) const
{
    if( !( qid >= 1 && ( relevant == -1 || relevant == +1 || relevant == 0 ) ) ) {
	std::cerr <<" scan_lift/irbase.cpp ERROR PRINTING DOC = " << docid << " qid = " << qid <<" relevant = " << relevant << " \n";
	exit(0);
    }
    fprintf(f , "%d qid:%d",relevant,qid);
    EACH(it,Features) fprintf(f," %d:%lf", it->first, it->second );
    fprintf(f," # %d\n", docid);
    fflush( f );
}
