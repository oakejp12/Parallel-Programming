#ifndef TIME_H
#define TIME_H

void tic() ;
void toc() ;
double etime( ) ;

extern struct timeval start_time ;
extern struct timeval finish_time ;
extern struct timeval current_time ;

#endif /* end of include guard:  */
