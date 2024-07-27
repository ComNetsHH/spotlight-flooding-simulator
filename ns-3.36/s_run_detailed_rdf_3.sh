#!/bin/bash -l
#SBATCH -p ib 
#SBATCH --job-name="DETAILED_RATE_DECAY_FLOODING"             	 
#SBATCH --ntasks=1           
#SBATCH --cpus-per-task=1
#SBATCH --mem-per-cpu=32G
#SBATCH --array=0-9999
#SBATCH --mail-type=ALL
#SBATCH --mail-user="k.fuger@tuhh.de"
#SBATCH --time=10-12:00:00
#SBATCH --constraint=OS8
#SBATCH --output=/dev/null

# Execute simulation
pyenv shell 3.9.14
python3 run_detailed_rdf_experiment.py $SLURM_ARRAY_TASK_ID 20000

# Exit job
exit
