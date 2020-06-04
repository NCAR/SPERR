from libcpp.string cimport string
from numpy cimport uint16_t
cimport cython
cimport numpy as np
import numpy as np
from libc.string cimport memcpy
from libcpp.memory cimport unique_ptr
from libcpp.memory cimport make_unique
from libc.stdlib cimport malloc

ctypedef unique_ptr[double] buffer_type_d 

ctypedef fused  float_t:
	cython.float
	cython.double

cdef extern from "SPECK3D.h" namespace "speck":

	cdef cppclass SPECK3D:
		void set_dims( size_t, size_t, size_t )
		void set_max_coeff_bits( uint16_t )
		void set_bit_budget( size_t )
		void set_image_mean(double)
		double get_image_mean() const

		void get_dims( size_t&, size_t&, size_t& ) const

		int encode()
		int decode()
		int write_to_disk(const string& filename )
		int read_from_disk(const string& filename )
		void copy_coeffs[T]( const T*, size_t)
		const buffer_type_d &get_read_only_coeffs() const



cdef class PySPECK3D:
	cdef SPECK3D *_thisptr      # hold a C++ instance which we're wrapping
	def __cinit__(self):
		self._thisptr = new SPECK3D()
	def __dealloc__(self):
		del self._thisptr
	def set_dims(self, nx=1, ny=1, nz=1):
		self._thisptr.set_dims(nx,ny,nz)

	def set_bit_budget(self, n):
		self._thisptr.set_bit_budget(n)
	def set_image_mean(self, mean):
		self._thisptr.set_image_mean(mean)
	def get_image_mean(self ):
		return self._thisptr.get_image_mean()
	def set_max_coeff_bits(self, n):
		self._thisptr.set_max_coeff_bits(n)
	def get_dims(self):
		cdef size_t x = 1
		cdef size_t y = 1
		cdef size_t z = 1
		self._thisptr.get_dims(x,y,z)
		return[x,y,z]
	def encode(self):
		return self._thisptr.encode()
	def decode(self):
		return self._thisptr.decode()
	def write_to_disk(self, path):
		return self._thisptr.write_to_disk(path.encode('utf-8'))
	def read_from_disk(self, path):
		return self._thisptr.read_from_disk(path.encode('utf-8'))
	def copy_coeffs(self, float_t[:] a not None):
		#self._thisptr.copy_coeffs(&a[0],a.size)
		cdef const float *a32ptr = NULL
		cdef const double *a64ptr = NULL

		if float_t is float:
			a32ptr = <float *>&a[0]
			self._thisptr.copy_coeffs(&a32ptr[0],a.size)
		elif float_t is double:
			a64ptr = <double *>&a[0]
			self._thisptr.copy_coeffs(&a64ptr[0],a.size)
		else:
			raise TypeError

	def get_coeffs(self):
		dims = self.get_dims()
		print ('dims = ', dims)
		nx = dims[2]
		ny = dims[1]
		nz = dims[0]
		cdef double[:, :, ::1] a3d
		cdef double[:, ::1] a2d
		if (nz != 1):
			a3d = <double[:nz, :ny, :nx]>self._thisptr.get_read_only_coeffs().get()
			return np.asarray(a3d)
		else:
			a2d = <double[:ny, :nx]>self._thisptr.get_read_only_coeffs().get()
			return np.asarray(a2d)


cdef extern from "CDF97.h" namespace "speck":
	#ctypedef unique_ptr[cython.double] buffer_type_d 
	cdef cppclass CDF97:
		void set_dims( size_t, size_t, size_t )
		void copy_data[T]( const T*, size_t)
		void dwt3d()
		void idwt3d()
		double get_mean() const
		void set_mean(double m) 
		const buffer_type_d& get_read_only_data() const

cdef class PyCDF97:
	cdef CDF97 *_thisptr      # hold a C++ instance which we're wrapping
	cdef list dims 
	def __cinit__(self):
		self._thisptr = new CDF97()

	def __dealloc__(self):
		del self._thisptr

	def set_dims(self, nx=1, ny=1, nz=1):

		# CDF97 API for getting dims is messed up. Make our own copy 
		#
		self.dims = [nz,ny,nx]
		self._thisptr.set_dims(nx,ny,nz)

	def copy_data(self, float_t[:] a not None):
		#self._thisptr.copy_data(&a[0],a.size)
		cdef const float *a32ptr = NULL
		cdef const double *a64ptr = NULL

		if float_t is float:
			a32ptr = <float *>&a[0]
			self._thisptr.copy_data(a32ptr,a.size)
		elif float_t is double:
			a64ptr = <double *>&a[0]
			self._thisptr.copy_data(a64ptr,a.size)
		else:
			raise TypeError

	def get_mean(self):
		return self._thisptr.get_mean()

	def set_mean(self, m):
		self._thisptr.set_mean(m)

	def dwt3d(self):
		self._thisptr.dwt3d()

	def idwt3d(self):
		self._thisptr.idwt3d()

	def get_data(self):
		nx = self.dims[2]
		ny = self.dims[1]
		nz = self.dims[0]
		cdef double[:, :, ::1] a3d
		cdef double[:, ::1] a2d
		if (nz != 1):
			a3d = <double[:nz, :ny, :nx]>self._thisptr.get_read_only_data().get()
			return np.asarray(a3d)
		else:
			a2d = <double[:ny, :nx]>self._thisptr.get_read_only_data().get()
			return np.asarray(a2d)
