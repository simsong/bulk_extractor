
#define CATCH_CONFIG_MAIN
#include "config.h"

#ifndef VERSION
#define VERSION "NO VERSION"
#endif


#include "tests/catch.hpp"

#include "hash_t.h"
#include "dfxml_writer.h"
#include "cpuid.h"

const uint8_t nulls[512] = {0};

int count_wrongs(void) {
    /* First test the operation of the digest function */
    uint8_t buf20[20] = {0,1,2,3,4,5,6,7,8,9,
                         10,11,12,13,14,15,16,17,18,19};
    int wrongs = 0;
    dfxml::sha1_t v20(buf20);
    for(int i=0;i<20;i++){
        if (v20.digest[i] != i) wrongs += 1;
    }
    return wrongs;
}

/****************************************************************
 *** test DFXML writer
 ****************************************************************/

bool test_dfxml_writer( void ){
    const int argc = 1;
    char **argv = 0;

    argv    = (char **)calloc(1,sizeof(char *));
    argv[0] = strdup("testapp");

    dfxml_writer *dw = new dfxml_writer("/tmp/output.xml", true);
    dw->add_DFXML_execution_environment("test program");
    dw->add_DFXML_creator("test", VERSION, "not git commit", argc, argv);
    dw->add_rusage();
    dw->comment("That's all folks.");
    dw->close();
    delete dw;
    return true;
}

TEST_CASE("dfxml_writer", "[vector]" ) {
    REQUIRE( test_dfxml_writer() == true );
}

TEST_CASE("hash_generator", "[vector]") {
    std::cout << "hash implementation: " << dfxml::digest_implementation_name() << std::endl;
    REQUIRE( count_wrongs() ==  0 );
    REQUIRE( dfxml::md5_generator::hash_buf(nulls,0).hexdigest()
             == "d41d8cd98f00b204e9800998ecf8427e" );
    REQUIRE( dfxml::sha1_generator::hash_buf(nulls,0).hexdigest()
             == "da39a3ee5e6b4b0d3255bfef95601890afd80709" );
    REQUIRE( dfxml::sha256_generator::hash_buf(nulls,0).hexdigest() ==
             "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855" );

#ifdef HAVE_SHA512_T
    REQUIRE( dfxml::sha512_generator::hash_buf(nulls,0).hexdigest() ==
             "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e" );
#endif
}


TEST_CASE("cpuid", "[cpuid]") {
#if defined(_WIN32) || defined(HAVE_ASM_CPUID)
    REQUIRE( CPUID::vendor() != "" );
    REQUIRE( CPUID::vendor().size() == 12 );
#else
    // CPUID is an x86 instruction; unsupported architectures report no vendor.
    REQUIRE( CPUID::vendor().empty() );
#endif
}
