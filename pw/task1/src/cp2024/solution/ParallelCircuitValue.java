package cp2024.solution;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;

import cp2024.circuit.Circuit;
import cp2024.circuit.CircuitValue;

public class ParallelCircuitValue implements CircuitValue {
    private ParallelCircuitSolver solver;
    private ExecutorService executor;
    private Circuit circuit;
    private PararellNodeValue rootNodeValue;
    private Future<Boolean> future;

    public ParallelCircuitValue(Circuit c, ExecutorService executor, ParallelCircuitSolver solver) {
        this.circuit = c;
        this.executor = executor;
        this.solver = solver;

        try {
            this.rootNodeValue = new PararellNodeValue(circuit.getRoot(), executor);
        } catch (Exception e) {
            System.err.println("Initizalization of ParallelCircuitvalue has failed!");
        }
        this.future = this.executor.submit(this.rootNodeValue);
    }

    @Override
    public boolean getValue() throws InterruptedException {
        if (!this.future.isDone() && this.solver.isInterrupted()) {
            throw new InterruptedException();
        }
        try {
            return this.future.get();
        } catch (ExecutionException e) {
            throw new InterruptedException();
        }
    }
}
