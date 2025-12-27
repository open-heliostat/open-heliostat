<script lang="ts">
	import Info from '~icons/tabler/info-circle';
	import type { StepperControl, StepperDiag } from '$lib/types/models';
	import Button from './Button.svelte';

	class StepperStatus {
		stst: boolean = false;
		olb: boolean = false;
		ola: boolean = false;
		s2gb: boolean = false;
		s2ga: boolean = false;
		s2vsb: boolean = false;
		s2vsa: boolean = false;
		otpw: boolean = false;
		ot: boolean = false;
		connected: boolean = false;
		infoclass: string = "";
	}

	function readStatus(stepper: StepperControl | StepperDiag) {
        let status = new StepperStatus;
        let mainFlags = stepper.status >>> 24;
        status.stst = (mainFlags & 0b10000000) > 0;
        status.olb = (mainFlags & 0b01000000) > 0;
        status.ola = (mainFlags & 0b00100000) > 0;
        status.s2gb = (mainFlags & 0b00010000) > 0;
        status.s2ga = (mainFlags & 0b00001000) > 0;
        status.otpw = (mainFlags & 0b00000100) > 0;
        let otherFlags = stepper.status >>> 24;
        status.s2vsb = (otherFlags & 0b10) > 0;
        status.s2vsa = (otherFlags & 0b01) > 0;
        status.connected = !(stepper.version == 0 || stepper.version == 255);
        status.infoclass = status.connected ? ((status.ola || status.olb || status.otpw || status.ot || status.s2ga || status.s2gb || status.s2vsa || status.s2vsb) ? 'alert-warning' : 'alert-info') : 'alert-warning';
        return status;
	}

	export let stepperControl: StepperControl | StepperDiag;
    let stepperStatus: StepperStatus;
	$: stepperStatus = readStatus(stepperControl);

    export let enableCb: () => void = () => {};
    export let encoderError = false;

</script>

<div class="alert {stepperStatus.infoclass} my-2 shadow-lg">
    <Info class="h-6 w-6 flex-shrink-0 stroke-current" />
    <span>
        Stepper driver {stepperStatus.connected ? (stepperControl.isEnabled ? 'enabled' : 'disabled') : 'disconnected'} !
        {#if !stepperStatus.connected}
        <br> Communication error, check the driver and power supply connexions.
        {:else}
            {#if stepperStatus.ola || stepperStatus.olb}
            <br> Open load detected, check motor connexions.
            {/if}
            {#if stepperStatus.s2ga || stepperStatus.s2gb}
            <br> Short circuit to ground detected, check motor connexions.
            {/if}
            {#if stepperStatus.s2vsa || stepperStatus.s2vsb}
            <br> Short circuit to supply detected, check motor connexions.
            {/if}
            {#if stepperStatus.ot}
            <br> Driver is too hot, let it cool down and retry.
            {:else if stepperStatus.otpw}
            <br> Driver temperature pre-warning, you may want to try lowering the current and/or reducing the load.
            {/if}
        {/if}
        {#if encoderError}
            <br> Encoder seems disconnected !
        {/if}
    </span>
    {#if !stepperControl.isEnabled}
        <Button label="Enable" onClick={enableCb}></Button>
    {/if}
</div>