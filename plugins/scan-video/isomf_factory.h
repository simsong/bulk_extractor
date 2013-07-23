#ifndef _ISOMF_FACTORY_H_
#define _ISOMF_FACTORY_H_

#include <iostream>
#include "isomf.h"

namespace isomf {

// factory class to generaet parameter set

class parameter_set_factory
{
  public:
    static sequence_parameter_set* get_avc_sps();
    static picture_parameter_set* get_avc_pps();
};

// factory class to generaet boxes 
class box_factory
{
  public:
    static es_descriptor_box* get_esds();

    static sample_description_box* 
        get_avc_stsd(unsigned width, unsigned height);

    static sample_description_box* 
        get_mp4a_stsd();

    static sample_description_box* 
        get_mp4v_stsd(unsigned width, unsigned height);

    static sample_description_box* 
        get_h263_stsd(unsigned width, unsigned height);

    static void print_stsd(sample_description_box* box);
};

}; // namespace isomf

#endif
