/* 
 * Program: psychometric.c
 * Author: Paul L. Gribble
 * http://www.gribblelab.org
 * April 4, 2013
 * compile with: gcc -Wall -o psychometric psychometric.c nmsimplex.c
 * loads in an ascii data file (2 columns: col1 = x position, col2 = response {0,1})
 * and fits a psychophysical function using binomial model with logistic link function
 * spits out bias (50% point), slope at 50%,
 * and acuity (middle 50th percentile distance, i.e. 75th - 25th)
 */
 
 #include "nmsimplex.h"
 #include <stdio.h>
 #include <math.h>
 #include <time.h>
 #include <string.h>

/* ============= DATA STRUCTURE AND UTILITY FUNCTIONS TO STORE DATA LOADED FROM FILE ============= */
//
// a struct to hold the data
 typedef struct {
	double **data; // data[0] = lateral hand position, data[1] = binary responses {0,1} = {left,right}
	int data_n;    // number of trials
} datastruct;

// utility function to initialize our datastruct
datastruct *datastruct_allocate(int n) {
	datastruct *thedata = malloc(sizeof(datastruct));
	thedata->data_n = n;
	thedata->data = malloc(sizeof(double)*2);
	thedata->data[0] = malloc(sizeof(double)*n);
	thedata->data[1] = malloc(sizeof(double)*n);
	return thedata;
}

// utility function to free our datastruct
void datastruct_free(datastruct *thedata) {
	free(thedata->data[0]);
	free(thedata->data[1]);
	free(thedata->data);
	free(thedata);
}

// utility function to count number of rows of the data file
int count_rows(char fname[]) {
	FILE *fid = fopen(fname, "r");
	if (fid != NULL) {
		int count = 0;
		char buf[1024];
		while (!feof(fid)) {
			fgets(buf,sizeof(buf),fid);
			count++;
		}
		count --;
		fclose(fid);
		return count;
	}
	else {
		return 0;
	}
}

// utility function to load the data file into a datastruct
void *load_data(char fname[]) {
	int n = count_rows(fname);
	if (n==0) {
		printf("ERROR LOADING FILE %s\n", fname);
		return NULL;
	}
	else {
		printf("found %d rows of data in %s\n", n, fname);
		datastruct *thedata = datastruct_allocate(n);
		FILE *fid = fopen(fname,"r");
		int i;
		for (i=0; i<thedata->data_n; i++) {
			fscanf(fid, "%lf %lf\n", &thedata->data[0][i], &thedata->data[1][i]);
		}
		fclose(fid);
		return thedata;
	}
}
/* =============================================================================================== */


/* =========================== THE LOGISTIC LINK FUNCTION AND ITS INVERSE =========================== */
//
// logistic link function
double logistic(double y)
{
	return 1 / (1 + exp(-y));
}

// inverse logistic function
double i_logistic(double p, double b[])
{
	return (log(-p / (p - 1)) - b[0]) / b[1];
}
/* =============================================================================================== */


/* ======== OUR OBJECTIVE FUNCTION TO CALCULATE LIKELIHOOD OF DATA GIVEN A CANDIDATE MODEL ======= */
//
 // negative log-likelihood of the data given
 // 	the model (parameter vector x[])
 //		extra stuff (void *extra)
double nll(double x[], void *extra)
{
	int i;
	double neg_log_lik = 0.0;
	double pos, p, y;
	int r;
	double **data = ((datastruct *)extra)->data;
	double data_n = ((datastruct *)extra)->data_n;
	for (i=0; i<data_n; i++) {
		pos = data[0][i];
		r = (int) data[1][i];
		y = x[0] + x[1]*pos;
 		p = logistic(y); // logistic link function
 		if (p>=1.0) {p = 1.0 - 1e-10;} // avoid numerical nasties
 		if (p<=0.0) {p = 1e-10;}
 		if (r==1) { neg_log_lik -= log(p); }
 		else { neg_log_lik -= log(1-p); }
 	}
 	return neg_log_lik;
 }
/* =============================================================================================== */
 

/* ============== LITTLE UTILITY FUNCTIONS TO FIND THE MINIMUM & MAXIMUM OF AN ARRAY ============= */
//
 double minarray(double *data, int n)
 {
 	double min = data[0];
 	int i;
 	for (i=0; i<n; i++) {
 		if (data[i] < min) { min = data[i]; }
 	}
 	return min;
 }

 double maxarray(double *data, int n)
 {
 	double max = data[0];
 	int i;
 	for (i=0; i<n; i++) {
 		if (data[i] > max) { max = data[i]; }
 	}
 	return max;
 }
/* =============================================================================================== */




 /* ====================================== THE MAIN FUNCTION ===================================== */
//
 int main(int argc, char *argv[])
 {
 	int ndist;
 	if (argc < 3) {
 		printf("\nUSAGE: ./psychometric fname_data ndist\n\n");
 		return 1;
 	}
 	else {
 		ndist = atoi(argv[2]);
 		datastruct *thedata = load_data(argv[1]);
 		if (thedata == NULL) {
 			return 1;
 		}
 		else {

 			// data loaded, let's go!
		 	double b[2]; // to store the 2 logistic coefficients
		 	srand ((unsigned)time(NULL));      // initialize random seed based on system time
		 	b[0] = (double) rand() / RAND_MAX; // 1st rand() call is not so random (why?)
		 	b[0] = (double) rand() / RAND_MAX; // initialize starting guess to random values between 0 and 1
		 	b[1] = (double) rand() / RAND_MAX;

		 	double min;

		 	min = simplex(nll, b, 2, 1.0e-8, 1, NULL, thedata); // find b[] to minimize nll()

		 	// print some junk to the screen
		 	printf("***************************************************************\n");
		 	printf("y = %7.5f + (%7.5f * x)\n", b[0], b[1]);
		 	printf("p(r|x) = 1 / (1 + exp(-y))\n");
		 	printf("***************************************************************\n");
		 	printf("bias = %7.5f\n", -b[0]/b[1]);
		 	printf("slope at 50%% = %7.5f\n", b[1]/4);
		 	printf("acuity (x75 - x25) = (%7.5f - %7.5f) = %7.5f\n", i_logistic(0.75, b), i_logistic(0.25, b), i_logistic(0.75, b)-i_logistic(0.25, b));
		 	printf("***************************************************************\n");

		 	// construct filenames to store modelparams and modelpred
		 	char fn_modelparams[256];
		 	strcat(fn_modelparams, argv[1]);
		 	strcat(fn_modelparams, "_");
		 	strcat(fn_modelparams, "params");
		 	char fn_dist[256];
		 	strcat(fn_dist, argv[1]);
		 	strcat(fn_dist, "_");
		 	strcat(fn_dist, "dist");
		 	char fn_modelpred[256];
		 	strcat(fn_modelpred, argv[1]);
		 	strcat(fn_modelpred, "_");
		 	strcat(fn_modelpred, "pred");

	 		printf("gnuplot commands to plot result:\n");
	 		printf("set yrange [-.05:1.15]\n");
	 		printf("plot '%s' using 1:($2 + (rand(0)/20)) title 'data' with points, \\\n     '%s' using 1:2 title 'model' with lines\n", argv[1], fn_modelpred);
	 		printf("***************************************************************\n");

		 	// open the output files for writing
		 	FILE *fid_modelparams = fopen(fn_modelparams, "w");
		 	FILE *fid_modelpred = fopen(fn_modelpred, "w");
		 	if ((fid_modelparams == NULL) | (fid_modelpred == NULL)) {

		 		printf("error opening output file for writing\n");
		 	}
		 	else {
		 		
		 		// output model predicted values to file
		 		double xmin = minarray(thedata->data[0], thedata->data_n);
		 		double xmax = maxarray(thedata->data[0], thedata->data_n);
		 		int npts = 50;
		 		int i;
		 		double xi, yi, pi;
		 		double xinc = ((xmax-xmin)/(npts-1));
		 		for (i=0; i<npts; i++) {
		 			xi = (xinc * i) + xmin;
		 			yi = b[0] + (b[1] * xi);
		 			pi = logistic(yi);
		 			fprintf(fid_modelpred, "%7.5f %7.5f\n", xi, pi);
		 		}
		 		fclose(fid_modelpred);
		 		
		 		// output model parameters to file
		 		fprintf(fid_modelparams, "%7.5f %7.5f %7.5f %7.5f %7.5f %7.5f %7.5f\n",
		 			b[0], b[1], -b[0]/b[1], b[1]/4, i_logistic(0.75, b), i_logistic(0.25, b), i_logistic(0.75, b)-i_logistic(0.25, b));
				fclose(fid_modelparams);

		 		// estimates of parameter distributions by simulating responses at each x value
		 		if (ndist > 0) {

				 	FILE *fid_dist = fopen(fn_dist, "w");
				 	if ((fid_dist) == NULL) {

				 		printf("error opening output file for writing\n");
				 	}
				 	else {

			 			printf("simulating %d times...\n", ndist);
			 			int j;
			 			double bmin, bb[2], pdist, ydist, rdist;

			 			// make a copy of the data
			 			datastruct *distdata = datastruct_allocate(thedata->data_n);
			 			for (j=0; j<distdata->data_n; j++) {
			 				distdata->data[0][j] = thedata->data[0][j];
			 				distdata->data[1][j] = thedata->data[1][j];
			 			}

			 			// simulate the experiment ndist times
			 			for (i=0; i<ndist; i++) {
					 		bb[0] = b[0]; // starting guess
					 		bb[1] = b[1];
					 		// for each x point
					 		for (j=0; j<distdata->data_n; j++) {
					 			ydist = b[0] + (b[1] * distdata->data[0][j]);
					 			pdist = logistic(ydist);
					 			// simulate the response
					 			rdist = (double) rand() / RAND_MAX;
					 			if (rdist <= pdist) {
					 				distdata->data[1][j] = 1.0;
					 			}
					 			else {
					 				distdata->data[1][j] = 0.0;
					 			}
					 		}

					 	// re-estimate the psychophysical curve based on the simulated response data
					 	bmin = simplex(nll, bb, 2, 1.0e-8, 1, NULL, distdata);

					 	// output the new estimates to the output file
					 	fprintf(fid_dist, "%7.5f %7.5f %7.5f %7.5f %7.5f %7.5f %7.5f\n",
					 		bb[0], bb[1], -bb[0]/bb[1], bb[1]/4, i_logistic(0.75,bb), i_logistic(0.25,bb), i_logistic(0.75,bb)-i_logistic(0.25,bb));
						}
						printf("done\n");
				 		datastruct_free(distdata);
						fclose(fid_dist);
					}
				}
			}
			datastruct_free(thedata);
		}
	}
	return 0;
}
/* =============================================================================================== */



















