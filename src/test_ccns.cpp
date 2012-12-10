int main(int argc,char **argv)
{
    char buf[65536];
    while(fgets(buf,sizeof(buf),stdin)){
	char buf2[65536];
	strcpy(buf2,buf);
	struct fef fef;
	if(fef_decode(buf,"CCN",&fef)){
	    if(test_ccn(&fef)){
		fputs(buf2,stdout);
	    }
	}
    }
}
