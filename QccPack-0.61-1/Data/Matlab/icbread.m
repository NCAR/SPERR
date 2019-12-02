function A = icbread(filename)
% A = ICBREAD(FILENAME)
% 
% Reads the QccPack ICB-format file specified by FILENAME into A.
%
% A is returned as a double array of size
%   num_frames x num_rows x num_cols
%

fid = fopen(filename, 'r', 'ieee-be');
header_value = fgetl(fid);
magic_num = sscanf(header_value, '%[A-Z]');

if (magic_num ~= 'ICB')
  error([filename ' is not an ICB file'])
end

num_cols = fscanf(fid, '%d', 1);
num_rows = fscanf(fid, '%d', 1);
num_frames = fscanf(fid, '%d', 1);
min_val = fscanf(fid, '%f', 1);
max_val = sscanf(fgets(fid), '%f', 1);

data = fread(fid, num_frames * num_rows * num_cols, 'float32');

frame_size = num_rows * num_cols;
index = 1;
A = zeros([num_frames num_rows num_cols]);
for frame = 1:num_frames
  A(frame, 1:num_rows, 1:num_cols) = ...
      reshape(data(index:(index + frame_size - 1)), num_cols, num_rows)';
  index = index + (num_rows * num_cols);
end

fclose(fid);
