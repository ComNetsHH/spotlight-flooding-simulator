import sys, os, math
import subprocess
import numpy as np

def get_params(run_idx = 0):
    params = []

    # for run in range(10):
    #     for send_interval in [0.06, 0.09, 0.12, 0.15, 0.18, 0.21, 0.24, 0.27, 0.3, 0.33, 0.36, 0.39, 0.42, 0.45]:
    #         for q in [1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0, 2.1]:
    #             for num_nodes in range(100, 520, 20):
    #                 params.append({'run': run, 'num_nodes': num_nodes, 'send_interval': send_interval, 'q': q})

    for num_nodes in [175]:
        for run in range(24, 36):
            for send_interval in np.arange(30, 300, 5):
                for q in np.arange(0, 3, 0.05):
                    params.append({'run': run, 'num_nodes': num_nodes, 'send_interval': send_interval/ 1000, 'q': q})

    print(f'running {run_idx} of {len(params)}')
    if run_idx >= len(params):
        return None
    return params[run_idx]

if __name__ == '__main__':
    run_command = 'spot-light-flooding' 
    run_idx = int(sys.argv[1])
    offset = int(sys.argv[2])
    params = get_params(run_idx + offset)
    print(params)
    v = 49

    if params != None:

        study_name = f'./res/v{v}'
        os.makedirs(f'{study_name}', exist_ok=True)

        num_nodes = params.get('num_nodes')
        send_interval = params.get('send_interval')
        run = params.get('run')
        q = params.get('q')
        density = 12
        warmup = 5
        A = num_nodes / density
        size = math.sqrt(A) * 1000
        simTime = size/33.3 + warmup
        #simTime = 180 + warmup

        res_file_name = f'kpi_rdf_n{num_nodes}_i{int(send_interval*1000)}_q{int(q*100)}_r{run}'
        if not os.path.isfile(f'./res/v{v}_parsed/summary_{res_file_name}.json'):
            command = ['./ns3', 'run', run_command, '--']
            command.append(f'--interval={send_interval}')
            command.append(f'--numNodes={num_nodes}')
            command.append(f'--seed={run}')
            command.append(f'--decayFactor={q}')
            command.append(f'--v={v}')
            command.append(f'--size={size}')
            command.append(f'--simTime={simTime}')
            command.append('--speedMin=22.2')
            command.append('--speedMax=33.3')
            command.append('--tracing=0')

            subprocess.run(command)
            subprocess.run(['python3', 'analysis_scripts/parse_results_v4.py', f'{v}', res_file_name])
