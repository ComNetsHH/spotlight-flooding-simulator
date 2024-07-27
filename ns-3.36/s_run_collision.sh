#!/bin/bash -l
#SBATCH -p ib 
#SBATCH --job-name="COLLISION"             	 
#SBATCH --ntasks=1           
#SBATCH --cpus-per-task=1
#SBATCH --mem-per-cpu=16G
#SBATCH --array=0-5949
#SBATCH --mail-type=ALL
#SBATCH --mail-user="k.fuger@tuhh.de"
#SBATCH --time=5-12:00:00
#SBATCH --constraint=OS8

# Execute simulation
python3 run_collision_experiment.py "collision-rate" $SLURM_ARRAY_TASK_ID

# Exit job
exit
