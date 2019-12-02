function icbwrite(filename, A)
% ICBWRITE(FILENAME, A)
%
% Writes the image cube A to FILENAME as a QccPack ICB-format
% file. A should be an array of size
%   num_frames x num_rows x num_cols
%
% Note that the values in A are cast as doubles before being written
% to filename as 32-bit floating-point numbers, as this is the
% requisite format for ICB files.
%

if (ndims(A) ~= 3)
  error('Array must be three-dimensional');
end

fid = fopen(filename , 'w', 'ieee-be');

[num_frames num_rows num_cols] = size(A);

fprintf(fid, 'ICB0.50\n');
fprintf(fid, '%d %d %d\n', [num_cols num_rows num_frames]);
fprintf(fid, '% 16.9e % 16.9e\n', double([min(min(min(A))) max(max(max(A)))]));

for frame = 1:num_frames
  fwrite(fid, ...
      double(reshape(reshape(A(frame, :, :), num_rows, num_cols)', ...
      (num_rows * num_cols), 1)), ...
      'float32');
end

fclose(fid);
