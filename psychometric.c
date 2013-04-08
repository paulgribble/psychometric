/* 
 * Program: psychometric.c
 * Author: Paul L. Gribble
 * http://www.gribblelab.org
 * April 4, 2013
 * compile with: gcc -Wall -o psychometric psychometric.c nmsimplex.c
 * loads in an ascii data file (2 columns: col1 = x position, col2 = response {0,1})
 * and fits a psychophysical function using binomial model with logit link function
 * spits out bias (50% point), slope at 50%,
 * and acuity (middle 50th percentile distance, i.e. 75th - 25th)
 */
 
 #include "nmsimplex.h"
 #include <stdio.h>
 #include <math.h>
 #include <time.h>

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
		printf("found %d rows of data in screen.cal\n", n);
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


/* =========================== THE LOGIT LINK FUNCTION AND ITS INVERSE =========================== */
//
// logit link function
double logit(double y)
{
	return 1 / (1 + exp(-y));
}

// inverse logit function
double i_logit(double p, double b[])
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
 		p = logit(y); // logit link function
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
 	if (argc < 3) {
 		printf("USAGE: ./psychometric fname_data fname_output\n");
 		return 1;
 	}
 	else {
 		datastruct *thedata = load_data(argv[1]);
 		if (thedata == NULL) {
 			return 1;
 		}
 		else {
		 	double b[2]; // to store the 2 logistic coefficients
		 	srand ((unsigned)time(NULL));
		 	b[0] = (double) rand() / RAND_MAX; // 1st rand() call is not so random (why?)
		 	b[0] = (double) rand() / RAND_MAX; // initialize starting guess to random values between 0 and 1
		 	b[1] = (double) rand() / RAND_MAX;
		 	printf("XXXXX-----> %8.5f, %8.5f\n", b[0], b[1]);
		 	double min = simplex(nll, b, 2, 1.0e-8, 1, NULL, thedata); // find b[] to minimize nll()
		 	printf("min = %8.5f\n", min);
		 	printf("\n***************************************************************\n");
		 	printf("y = %8.5f + (%8.5f * x)\n", b[0], b[1]);
		 	printf("p(r|x) = 1 / (1 + exp(-y))\n");
		 	printf("***************************************************************\n");
		 	printf("bias = %8.5f\n", -b[0]/b[1]);
			printf("slope at 50%% = %8.5f\n", b[1]/4);
		 	double x25 = i_logit(0.25, b);
		 	double x75 = i_logit(0.75, b);
		 	printf("acuity (x75 - x25) = (%8.5f - %8.5f) = %8.5f\n", x75, x25, x75-x25);
		 	printf("***************************************************************\n");
		 	FILE *fid = fopen(argv[2], "w");
		 	if (fid == NULL) {
		 		printf("error opening output file for writing%s\n", argv[2]);
		 	}
		 	else {
		 		// output predicted values to output file
		 		double xmin = minarray(thedata->data[0], thedata->data_n);
		 		double xmax = maxarray(thedata->data[0], thedata->data_n);
		 		int npts = 50;
		 		int i;
		 		double xi, yi, pi;
		 		double xinc = ((xmax-xmin)/(npts-1));
		 		for (i=0; i<npts; i++) {
		 			xi = (xinc * i) + xmin;
		 			yi = b[0] + (b[1] * xi);
		 			pi = logit(yi);
		 			fprintf(fid, "%8.5f %8.5f\n", xi, pi);
		 		}
		 		fclose(fid);
		 		printf("gnuplot commands to plot result:\n\n");
		 		printf("set yrange [-.05:1.15]\n");
		 		printf("plot '%s' using 1:($2 + (rand(0)/20)) title 'data' with points, \\\n     '%s' using 1:2 title 'model' with lines\n", argv[1], argv[2]);
		 		printf("\n***************************************************************\n");
		 	}
		 	datastruct_free(thedata);
		 }
		}
		return 0;
	}
/* =============================================================================================== */


