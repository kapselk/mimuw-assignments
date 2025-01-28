package cp2024.solution;

import cp2024.circuit.CircuitSolver;
import cp2024.circuit.CircuitValue;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import cp2024.circuit.Circuit;

public class ParallelCircuitSolver implements CircuitSolver {
    private boolean interrupted = false;
    private ExecutorService executor;
    private List<ParallelCircuitValue> circuitValues;

    public ParallelCircuitSolver() {
        this.executor = Executors.newCachedThreadPool();
        this.circuitValues = new ArrayList<ParallelCircuitValue>();
    }

    @Override
    public CircuitValue solve(Circuit c) {
        if (isInterrupted()) {
            return new ParallelBrokenCircuitValue();
        }
        ParallelCircuitValue circuitValue = new ParallelCircuitValue(c, executor, this);
        circuitValues.add(circuitValue);
        return circuitValue;
    }

    @Override
    public void stop() {
        this.interrupted = true;
        executor.shutdownNow();
    }

    public boolean isInterrupted() {
        return this.interrupted;
    }
}
