import sys
import math
import subprocess

def get_params(run_idx=0):
    params = []
    for run in range(0, 100):
        for num_nodes in [300]:
            for nodes_per_container in range(5, 51, 5):  # Starting from 5 to 150, increment by 5
                params.append({'run': run, 'num_nodes': num_nodes, 'nodes_per_container': nodes_per_container})

    print(f'running {run_idx} of {len(params)}')
    if run_idx >= len(params):
        return None
    return params[run_idx]

if __name__ == '__main__':
    run_command = 'spot-light-flooding'
    run_idx = int(sys.argv[1])
    offset = int(sys.argv[2])
    params = get_params(run_idx + offset)

    if params is not None:
        num_nodes = params['num_nodes']
        run = params['run']
        nodes_per_container = params['nodes_per_container']  # Retrieve nodesPerContainer from params
        density = 12
        warmup = 5
        A = num_nodes / density
        size = math.sqrt(A) * 1000
        simTime = size / 33.3 + warmup

        command = ['./ns3', 'run', run_command, '--']
        command.append(f'--numNodes={num_nodes}')
        command.append(f'--seed={run}')
        command.append(f'--size={size}')
        command.append(f'--simTime={simTime}')
        command.append(f'--nodesPerContainer={nodes_per_container}')  # Add nodesPerContainer to the command
        command.append(f'--Threshold=99000')

        subprocess.run(command)