function plotOutput(name, grid, algo, w_hat, measure, option);

N_values = [5 10 20 40 80];
dimension_values = [286 1771 12341 91881 708561];
N1_values = [11 23 41 92 164 308];
if grid == 1;
	N2_values = [128 510 1652 8556 26772 94900];
else	
	N2_values = [34 156 460 2248 6974 23898];
end;

statusCodes = {'+' '*' 'o'}; 
set(0, 'DefaultAxesColorOrder', [0 0 0], ...
		      'DefaultAxesLineStyleOrder', '-|-.|--|:');
plot_position = [0.63 0.63 28.45 19.72];
plot_position_save = [3 2 17 8.55];
threshold = 10;
testcases = 10;
measure_descr = {'rrn (residuum)', 'ren (SO(3))', 'rwen (S^2 x S^2)'};


data = nan*ones(length(N1_values), length(N_values), testcases);
err = nan*ones(length(N1_values), length(N_values));
status = ones(length(N1_values), length(N_values));


files = dir(sprintf('%s.*.%s.%s.*', name, algo, w_hat));
pattern = sprintf('%s.N_%%d.N1_%%d.N2_%%d.%s.%s.%%d-%%d', name, algo, w_hat);

for count=1:length(files);
	curFile = files(count).name;
	[N, N1, N2, firstcase, lastcase] = strread(curFile, pattern);
	N_ind = find(N == N_values);
	N1_ind = find(N1 == N1_values);
	N2_ind = find(N2 == N2_values);
	if N1_ind ~= N2_ind;
		disp(curFile);
		disp('Inconsistent N1 and N2!');
		disp(N1);
		disp(N2);
		return;
	end;	

	fid = fopen(curFile, 'r');

	results = textscan(fid, '%n%n%n', 'commentStyle', '#');
	results = results{measure};

	if length(results) ~= lastcase-firstcase+1 + 3;
		disp(curFile);
		disp('Inconsistent number of testcases!');
		disp(length(results)-3);
		disp(lastcase-firstcase+1);
	end;

	data(N1_ind, N_ind, (firstcase+1):(firstcase+length(results)-3)) = ...
																		 results(4:length(results));

	fclose(fid);
end;	
	
for N1_ind = 1:length(N1_values);
	for N_ind = 1:length(N_values);
		ind = find(data(N1_ind, N_ind, :) == data(N1_ind, N_ind, :));
		cur_data = data(N1_ind, N_ind, ind);
		
		if length(cur_data) < testcases;
			status(N1_ind, N_ind) = 3;
		end;	

		if length(cur_data) >= 1;

			err(N1_ind, N_ind) = geomean(cur_data);

			if geomean(cur_data)/min(cur_data) > threshold || ...
				max(cur_data)/geomean(cur_data) > threshold;

				status(N_ind, N1_ind) = 2;
			end;	
		end;
	end;
end;	

figure();

loglog(N1_values', err);

set(gca, 'XTick', N1_values);
set(gca, 'XTickLabel', sprintf('%d / %d|', [N1_values; N2_values])); 
set(gca, 'XLim', [8 350]);
set(gca, 'YLim', [1e-17 10]);
set(gca, 'XMinorTick', 'off');
set(gca, 'YMinorTick', 'on');

title(sprintf('Algorithm: %s', algo));
xlabel('N');
ylabel(sprintf('Error Measure: %s', measure_descr{measure}));

for N_ind = 1:length(N_values);
	N = N_values(N_ind);
	dimension = dimension_values(N_ind);
	leg{N_ind} = sprintf('L = %d, dimension = %d', N, dimension);
end;	
leg_handle = legend(leg, 'location', 'southoutside');

set(leg_handle, 'units', 'centimeters');
pos = get(leg_handle, 'position');
lwidth = pos(3);
	
set(gcf, 'units', 'centimeters');	
pos = get(gcf, 'position');
pos(3) = pos(3) + lwidth;
set(gcf, 'position', pos);

for code = 1:3
	ind = find(status ~= code);

	err_code = err;
	err_code(ind) = nan;

	hold on;
	loglog(N1_values', err_code, statusCodes{code});
end;

set(gcf, 'units', 'centimeters');

if strcmp(option, 'print');
	set(gcf, 'paperOrientation', 'landscape');
	set(gcf, 'paperPosition', plot_position);
	print -Plaser1
end;	
if strcmp(option, 'save');
	set(gcf, 'paperOrientation', 'portrait');
	set(gcf, 'paperPosition', plot_position_save);
	saveas(gcf, sprintf('%s.%s.%s.%s.pdf', name, algo, w_hat, measure_descr{measure}));
end;	
set(gcf, 'paperOrientation', 'landscape');
set(gcf, 'paperPosition', plot_position);