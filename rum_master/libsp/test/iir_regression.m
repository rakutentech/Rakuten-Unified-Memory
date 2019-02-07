function []=iir_regression()
% IIR regression
% Reads output from executable and verify operation.
%
% Rick Lan
% See LICENSE for license.

regress_biquad('regress_iirbiquad_float');
regress_biquad('regress_iirbiquad_fixed');
end


function []=regress_biquad(prefix)
fprintf('%s ...\n', prefix);
c=load([prefix, '_coeffs.txt']);
o=load([prefix, '_impulse_response.txt']);
b=c(1:3)*c(7);
a=c(4:6);
ref=impz(b,a,100);
fprintf('mse is %.12g\n', mse(ref,o));
end


function [m] = mse(ref, cmp)
%MSE Compute mean squared error.
%   Y = MSE(REF, NOISY)
%   Input data structure supported: scalar, vector.
%   Input data type supported: real, complex.
%
%   Rick Lan
%   See LICENSE for the license.
%
error(nargchk(2, 2, nargin));
e = cmp - ref;
m = mean( e.*conj(e) );
end