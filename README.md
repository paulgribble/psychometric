psychometric
============

Estimating Psychometric* Functions in C
---------------------------------------

*[psychometric function](http://en.wikipedia.org/wiki/Psychometric_function)

The program reads in an ascii data file with two columns:

* col 1 = variable (e.g hand position along left-right axis)
* col 2 = binary response (0,1) (e.g. "left" vs "right" sensed hand position)

and fits a binomial model + logit link function using [maximum likelihood estimation](http://en.wikipedia.org/wiki/Maximum_likelihood). A good tutorial on MLE is:

* [Tutorial on maximum likelihood estimation by IJ Myung](http://www.sciencedirect.com/science/article/pii/S0022249602000287)

The model is of the form:

	y = b0 + (b1 * x)
	p(x) = Pr(response|x) = 1 / (1 + exp(-y))

The model parameters b0 and b1 are found that minimize the negative log-likelihood of the data. This is done using numerical optimization. The [Nelder-Mead simplex algorithm](http://en.wikipedia.org/wiki/Nelder–Mead_method) is used here. The code makes use of [Michael Hutt's](http://www.mikehutt.com) implementation of the Nelder-Mead algorithm.

The program outputs to the screen:

* the model parameters b0 and b1,
* the bias (the x value at the 50th percentile) and
* the slope at the 50th percentile
* the acuity (the distance in x between the 25th and 75th percentile)

The program also generates two output files:

* _modelparams
	* row 1 = b0, b1, bias, slope, x75, x25, x72-x25
	* rows 2 -> nboot are re-estimates based on nboot simulations of the experimental responses
* _modelpred: p(x) for 50 x points across the range of input x

an example data file is exdata

	gcc -Wall -o psychometric psychometric.c nmsimplex.c
	./psychometric exdata 10000
	found 154 rows of data in exdata
	***************************************************************
	y = 0.60414 + (0.48712 * x)
	p(r|x) = 1 / (1 + exp(-y))
	***************************************************************
	bias = -1.24024
	slope at 50% = 0.12178
	acuity (x75 - x25) = (1.01511 - -3.49558) = 4.51069
	***************************************************************
	gnuplot commands to plot result:
	set yrange [-.05:1.15]
	plot 'exdata' using 1:($2 + (rand(0)/20)) title 'data' with points, \
	     'exdata_modelpred' using 1:2 title 'model' with lines
	***************************************************************
	bootstrapping 10000 times...
	done

An example of the graphic produced by the gnuplot commands for exdata is shown below. Note that the data are offset in y using random values, to help with visualization of the (binary) responses.

![Image](exdata_modelpred.gif)

You can have a look at the bootstrap distributions of the parameters like so: (here I use [GNU Octave](http://www.gnu.org/software/octave/) ):

	load exdata_modelparams
	figure
	subplot(2,1,1)
	hist(exdata_modelparams(:,3))
	xlabel('BIAS (mm)')
	ylabel('COUNT')
	subplot(2,1,2)
	hist(exdata_modelparams(:,7))
	xlabel('ACUITY (mm)')
	ylabel('COUNT')
	print exdata_modelparams.jpg -djpg

![Image](exdata_modelparams.jpg)
