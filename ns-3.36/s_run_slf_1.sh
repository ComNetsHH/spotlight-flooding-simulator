#!/bin/bash -l
#SBATCH -p ib
#SBATCH --job-name="SPOT_LIGHT_FLOODING"
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem-per-cpu=16G
#SBATCH --array=0-1000  # Adjusted to match the number of runs (20 runs * 17 different num_nodes = 340)
#SBATCH --mail-type=ALL
#SBATCH --mail-user="md.rasik@tuhh.de"
#SBATCH --time=2-12:00:00
#SBATCH --constraint=OS8
#SBATCH --output=slf_%A_%a.out  # Output file name with job array ID and task ID

# Load necessary environment (if needed)

pyenv shell 3.9.14

# Execute simulation
python3 slf.py $SLURM_ARRAY_TASK_ID 0

# Exit job
exit
