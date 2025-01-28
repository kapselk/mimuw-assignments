package cp2024.solution;

import java.util.ArrayList;
import java.util.Hashtable;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorCompletionService;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;

import cp2024.circuit.*;

public class PararellNodeValue implements Callable<Boolean> {
    private CircuitNode node;
    private ExecutorService executor;
    private Hashtable<Future<Boolean>, Integer> futureToId;
    private boolean[] valuesOfArgs;
    private boolean[] calculated;
    private int numberOfArgs;
    private Optional<Boolean> value;

    public PararellNodeValue(CircuitNode node, ExecutorService executor) throws InterruptedException {
        this.node = node;
        this.executor = executor;
        this.futureToId = new Hashtable<>();
        this.numberOfArgs = this.node.getArgs().length;
        this.valuesOfArgs = new boolean[this.numberOfArgs];
        this.calculated = new boolean[this.numberOfArgs];
        this.value = Optional.empty();
    }

    private Boolean isResolved() {
        if (this.node.getType() == NodeType.NOT) {
            if (this.calculated[0]) {
                this.value = Optional.of(!this.valuesOfArgs[0]);
                return true;
            }
            return false;
        }
        else if (this.node.getType() == NodeType.IF) {
            if (this.calculated[0]) { // condition calculated
                if (this.valuesOfArgs[0]) { // condition == true
                    if (this.calculated[1]) {
                        this.value = Optional.of(this.valuesOfArgs[1]);
                        return true;
                    }
                } else {
                    if (this.calculated[2]) {
                        this.value = Optional.of(this.valuesOfArgs[2]);
                        return true;
                    }
                }
            } else if (this.calculated[1] && this.calculated[2] && this.valuesOfArgs[1] == this.valuesOfArgs[2]) {
                this.value = Optional.of(this.valuesOfArgs[1]);
                return true;
            }
            return false;
        } else {
            int cntTrue = 0, cntFalse = 0;
            for (int i = 0; i < this.numberOfArgs; i++) {
                if (this.calculated[i]) {
                    if (this.valuesOfArgs[i]) cntTrue++;
                    else cntFalse++;
                }
            }
            NodeType nodeType = this.node.getType();
            if (nodeType == NodeType.AND) {
                if (cntFalse > 0) {
                    this.value = Optional.of(false);
                    return true;
                }  else if (cntTrue == this.numberOfArgs) {
                    this.value = Optional.of(true);
                    return true;
                }
            } else if (nodeType == NodeType.OR) {
                if (cntTrue > 0) {
                    this.value = Optional.of(true);
                    return true;
                } else if (cntFalse == this.numberOfArgs) {
                    this.value = Optional.of(false);
                    return true;
                }
            } else if (nodeType == NodeType.GT) {
                int threshold = ((ThresholdNode)this.node).getThreshold();
                if (cntTrue > threshold) {
                    this.value = Optional.of(true);
                    return true;
                } else if ((this.numberOfArgs - cntFalse) <= threshold) { // N - cntFalse = mxCntTrue <= x
                    this.value = Optional.of(false);
                    return true;
                }
            } else if (nodeType == NodeType.LT) {
                int threshold = ((ThresholdNode)this.node).getThreshold();
                if ((this.numberOfArgs - cntFalse) < threshold) {
                    this.value = Optional.of(true);
                    return true;
                } else if (cntTrue >= threshold) {
                    this.value = Optional.of(false);
                    return true;
                }
            }
            return false;
        }
    }

    @Override
    public Boolean call() throws Exception {
        if (node instanceof LeafNode) {
            Boolean val = false;
            val = ((LeafNode)node).getValue();
            return val;
        }
        ExecutorCompletionService<Boolean> completionService = new ExecutorCompletionService<>(this.executor);
        List<Future<Boolean>> futures = new ArrayList<>();        
        int id = 0;
        for (CircuitNode child : this.node.getArgs()) {
            PararellNodeValue childValue = new PararellNodeValue(child, executor);
            Future<Boolean> future = completionService.submit(childValue);
            futureToId.put(future, id++);
            futures.add(future);
        }

        try {
            for (int i = 0; i < this.node.getArgs().length; i++) {
                // we wait for this.node.getArgs().length arguments to calculate
                Future<Boolean> calculatedValue = completionService.take();
                Boolean value = calculatedValue.get(); // propagates exception
                int fId = futureToId.get(calculatedValue);
                this.valuesOfArgs[fId] = value;
                this.calculated[fId] = true;
                if (this.isResolved()) {
                    for (Future<Boolean> f : futures) {
                        f.cancel(true);
                    }
                    break;
                }
            }
        } catch (ExecutionException e) {
            ;
        } finally {
            for (Future<Boolean> f : futures) {
                if (!f.isDone()) {
                    f.cancel(true);
                }
            }
        }
        // value should be calculated
        return this.value.get();
    }
}