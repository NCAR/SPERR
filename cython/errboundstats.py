import xarray as xr
import matplotlib
import numpy as np
import matplotlib.pyplot as plt


import numpy as np
import argparse
from speck3d import PySPECK3D
from speck3d import PyCDF97


def rmse(a, aprime):
	return np.sqrt(np.mean((a-aprime)**2))

def nrmse(a, aprime):
	return rmse(a,aprime) / (np.amax(a) - np.amin(a))

def lmax(a, aprime):
	return np.amax(np.absolute(a-aprime))

def nlmax(a, aprime):
	return lmax(a,aprime) / (np.amax(a) - np.amin(a))

def print_stats(a):
	print('range [', np.amin(a), ' ', np.amax(a), ']')
	print('mean ', np.mean(a))

def abs_err_count(a, aprime, tol):
	tmp = (np.abs(a-aprime) > tol)
	c = np.sum(tmp)
	return c

def rel_err_count(a, aprime, tol):
	tmp = (np.abs((a-aprime)/a) > tol)
	c = np.sum(tmp)
	return c

def range_rel_err_count(a, aprime, tol):
	tmp = (np.abs((a-aprime)/(np.amax(a) - np.amin(a))) > tol)
	c = np.sum(tmp)
	return c


def effective_bpp(data, dataPrime, bpp, frep):
	f = err_count(data, dataPrime, args.tol) / float(data.size)
	return(((1.0-f) * float(bpp)) + (f * float(frep)))

def read_var(file, ftype, varname, dims=[1,1,1]):
	print('file = ', file)
	if ftype == "netcdf":
		ds = xr.open_dataset(file, decode_times=False)
		data = ds.get(varname).values[0,:,:,:]
	else:
		data = np.fromfile(args.file, dtype=np.float32, count=nelements)
		data = data.reshape(dims)

	return (data)

def plotit(data, dataPrime):
	
	num_bins = 250
	density = False

	fig, ax = plt.subplots()

	# the histogram of the data
	n, bins, patches = ax.hist(np.absolute(data-dataPrime).flatten(), num_bins, density=density)

	# Tweak spacing to prevent clipping of ylabel
	fig.tight_layout()
	plt.show()


parser = argparse.ArgumentParser()
parser.add_argument("file", help="raw binary input file")
parser.add_argument("-d", "--dims", type=int, default=[], nargs=3, help="Dimensions ordered slowest to fastest varying")
parser.add_argument("-b", "--bpp", type=float, default=1.0, help="bits per pixel")
parser.add_argument("-t", "--tol", type=float, default=0.01, help="Specify fixed accuracy in terms of absolute or relative error tolerance")
parser.add_argument("--errtype", type=str, default="rrel", help="Absolute or relative error (abs|rel|rrel)")
parser.add_argument("-f", "--frep", type=int, default=64, help="# bits to represent floating point # plus offset")
parser.add_argument("-v", "--var3d", type=str, help="Name of 3D variable (NetCDF only)")
parser.add_argument("--ftype", type=str, default="raw", help="File type (netcdf|raw)")
args = parser.parse_args()



if args.errtype == "abs":
	err_count = abs_err_count
elif args.errtype == "rel":
	err_count =  rel_err_count
elif args.errtype == "rrel":
	err_count =  range_rel_err_count

data = read_var(args.file, args.ftype, args.var3d, args.dims)
dims = data.shape

#print_stats(data)
#exit(0)

nelements = np.prod(dims)

total_bits = 32 * nelements 

cdf = PyCDF97()
cdf.set_dims(dims[2], dims[1], dims[0])
cdf.copy_data(data.flatten())
cdf.dwt3d()

encoder = PySPECK3D()
encoder.set_dims(dims[2], dims[1], dims[0])
encoder.set_image_mean(cdf.get_mean())
encoder.copy_coeffs(cdf.get_data().flatten())
encoder.set_bit_budget(total_bits)
encoder.encode()

file = 'foo.bits'
encoder.write_to_disk(file)

print_stats(data)
print ('Error type ', args.errtype)
print ('Error tolerance ', args.tol)
low = 0.0
high = 64.0
bpp = (high + low) / 2.0
best_nofail_bpp = high
best_fail_bpp = high
best_fail_count = data.size
for exp in range(0,10):

	total_bits = bpp * nelements 

	decoder = PySPECK3D()
	idwt = PyCDF97()

	print ('Bits per pixel ', bpp)
	decoder.read_from_disk(file);
	decoder.set_bit_budget( total_bits );
	decoder.decode();

	idwt.set_dims(dims[2], dims[1], dims[0])
	idwt.set_mean( decoder.get_image_mean() );
	idwt.copy_data( decoder.get_coeffs().flatten() );
	idwt.idwt3d();

	dataPrime = idwt.get_data()
	print_stats(dataPrime)

	nfailed = err_count(data, dataPrime, args.tol)

	print ('rmse ', rmse(data, dataPrime))
	print ('lmax ', lmax(data, dataPrime))
	print ('nrmse ', nrmse(data, dataPrime))
	print ('nlmax ', nlmax(data, dataPrime))
	print ('bpp, effective bpp, failed count, failed fraction ', bpp, ' ', effective_bpp(data, dataPrime, bpp, args.frep), nfailed, float(nfailed) / float(data.size))
	print ('')

	if nfailed == 0:
		best_nofail_bpp = bpp
	else:
		best_fail_bpp = bpp
		best_fail_count = nfailed

	if nfailed == 0:
		high = bpp
	else:
		low = bpp

	bpp = (high + low) / 2.0

	#plotit(data, dataPrime)


print(f'Variable\tNo fail bpp\tFail bpp\tFail count\tTotal count\tTolerance\tErr type')
print(f'{args.var3d}\t{best_nofail_bpp}\t{best_fail_bpp}\t{best_fail_count}\t{data.size}\t{args.tol}\t{args.errtype}')
