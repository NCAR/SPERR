import xarray as xr
import matplotlib
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


def read_var(file, ftype, varname, ts, dims=[1,1,1]):
	print('file = ', file)
	if ftype == "netcdf":
		ds = xr.open_dataset(file, decode_times=False)
		data = ds.get(varname).isel(time=ts).values
	else:
		data = np.fromfile(args.file, dtype=np.float32, count=nelements)
		data = data.reshape(dims)

	return (data)

def get_num_ts(file, ftype, varname):
	print('file = ', file)
	if ftype == "netcdf":
		ds = xr.open_dataset(file, decode_times=False)
		da = ds.get(varname)
		if 'time' in da.dims:
			return da.time.size
		else:
			return 1
	else:
		return(1)


parser = argparse.ArgumentParser()
parser.add_argument("file", help="raw binary input file")
parser.add_argument("-d", "--dims", type=int, default=[], nargs=3, help="Dimensions ordered slowest to fastest varying")
parser.add_argument("-b", "--bpp", type=float, default=1.0, help="bits per pixel")
parser.add_argument("-t", "--tol", type=float, default=0.01, help="Specify fixed accuracy in terms of absolute or relative error tolerance")
parser.add_argument("--errtype", type=str, default="rrel", help="Absolute or relative error (abs|rel|rrel)")
parser.add_argument("-f", "--frep", type=int, default=64, help="# bits to represent floating point # plus offset")
parser.add_argument("--ts", type=int, default=0, help="time step number")
parser.add_argument("-v", "--var3d", type=str, help="Name of 3D variable (NetCDF only)")
parser.add_argument("--ftype", type=str, default="raw", help="File type (netcdf|raw)")
args = parser.parse_args()



if args.errtype == "abs":
	err_count = abs_err_count
elif args.errtype == "rel":
	err_count =  rel_err_count
elif args.errtype == "rrel":
	err_count =  range_rel_err_count


data = read_var(args.file, args.ftype, args.var3d, args.ts, args.dims)
dims = data.shape

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

total_bits = args.bpp * nelements 

decoder = PySPECK3D()
idwt = PyCDF97()

print ('Bits per pixel ', args.bpp)
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
print ('bpp, failed count, failed fraction ', args.bpp, ' ', nfailed, float(nfailed) / float(data.size))
print ('')

