package cc.ioctl.nfcncihost.procedure;

import cc.ioctl.nfcncihost.procedure.step.EmptyStep;
import cc.ioctl.nfcncihost.procedure.step.EarlyInit;

public class StepFactory {

    public static final int STEP_EARLY_INIT = 10;

    public static Step getStep(int id) {
        switch (id) {
            case STEP_EARLY_INIT:
                return new EarlyInit();
            default:
                return new EmptyStep();
        }
    }
}
