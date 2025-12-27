<script lang="ts">
	import { onMount, onDestroy } from 'svelte';
    import Collapsible from '$lib/components/Collapsible.svelte';
	import type { ControllerState, StepperDiag } from '$lib/types/models'
	import Slider from '$lib/components/Slider.svelte';
	import Checkbox from '$lib/components/Checkbox.svelte';
	import GridForm from '$lib/components/GridForm.svelte';
	import Spinner from '$lib/components/Spinner.svelte';
	import StepperStatusComp from '$lib/components/StepperStatusComp.svelte';
	import ChartComp from '$lib/components/ChartComp.svelte';
	import SettingsCard from './SettingsCard.svelte';
	import { getJsonRest, postJsonRest } from '$lib/stores/rest';
	import DisableButton from './DisableButton.svelte';
	import StopButton from './StopButton.svelte';

    export let label: string;
    export let restPath : string;

    let controllerState : ControllerState;
    let calibrationOffsets : number[] | undefined;

    let intervalID: any;
    onMount(() => {
        intervalID = setInterval(()=>{
            getPosition();
        }, 987);
    });
    onDestroy(()=> {
        clearInterval(intervalID);
    });

    async function getPosition() {
        getJsonRest(restPath + "/position", controllerState.position).then((data) => controllerState.position = data);
    }

    async function getControllerState() {
        // const res = await fetch(restPath);
        // const data = await res.json();
        // controllerState = data;
        // console.log(data);
        // return data;
        return getJsonRest(restPath, controllerState).then((data) => controllerState = data);
    }

    function postControllerState() {
        postJsonRest(restPath, controllerState).then((data) => controllerState = data);
    }

    function getCalibrationOffsets() {
        return getJsonRest(restPath + '/calibration/offsets/', calibrationOffsets).then((data) => calibrationOffsets = data);
    }

    let calibrationIntervalID : any;

    function startCalibration() {
        if (controllerState.calibration) controllerState.calibration.running = true;
        calibrationIntervalID = setInterval(getCalibrationOffsets, 1011);
        postJsonRest(restPath + '/calibration/start', {});
    }
    
    function stopCalibration() {
        if (controllerState.calibration) controllerState.calibration.running = false;
        clearInterval(calibrationIntervalID);
        postJsonRest(restPath + '/calibration/stop', {})
        .then(() => getCalibrationOffsets());
    }

    function resetCalibration() {
        clearInterval(calibrationIntervalID);
        postJsonRest(restPath + '/calibration/reset', {});
        calibrationOffsets = [];
    }

    function saveCalibration() {
        stopCalibration();
        postJsonRest(restPath + '/calibration', {offsets: calibrationOffsets});
    }

</script>

<SettingsCard>
	<span slot="title">{label} Controller</span>
    {#await getControllerState()}
    <Spinner></Spinner>
    {:then nothing}
        <div>
            {#await fetch(restPath + '/stepper/diag').then(async (res) => await res.json())}
            <Spinner></Spinner>
            {:then data} 
                <StepperStatusComp 
                    stepperControl={data}
                    bind:encoderError={controllerState.encoderError}
                    enableCb={()=>{postJsonRest(restPath + '/stepper/config', {enabled: true})}}
                ></StepperStatusComp>
            {/await}
            <GridForm>
                <Slider 
                    label="Position" 
                    bind:value={controllerState.position}
                    min={controllerState.limits.enabled ? (controllerState.limits.begin < controllerState.limits.end ? controllerState.limits.begin : controllerState.limits.end) : 0} 
                    max={controllerState.limits.enabled ? (controllerState.limits.begin > controllerState.limits.end ? controllerState.limits.begin : controllerState.limits.end) : 360}
                    step={0.01}
                    disabled
                ></Slider>
                <Slider 
                    label="Target" 
                    bind:value={controllerState.target}
                    min={controllerState.limits.enabled ? (controllerState.limits.begin < controllerState.limits.end ? controllerState.limits.begin : controllerState.limits.end) : 0} 
                    max={controllerState.limits.enabled ? (controllerState.limits.begin > controllerState.limits.end ? controllerState.limits.begin : controllerState.limits.end) : 360}
                    step={0.01}
                    onChange={() => postJsonRest(restPath, {target: controllerState.target})}
                ></Slider>
            </GridForm>
            <Collapsible>
                <span slot="title">Settings</span>
                <span class="text-lg">Control</span>
                <GridForm>
                    <Checkbox 
                        label="Enable" 
                        bind:value={controllerState.enabled}
                        onChange={postControllerState}
                    ></Checkbox>
                    <Checkbox 
                        label="Invert" 
                        bind:value={controllerState.invert}
                        onChange={postControllerState}
                    ></Checkbox>
                    <Slider 
                        label="Offset" 
                        bind:value={controllerState.offset}
                        min={0} 
                        max={360} 
                        step={0.01}
                        onChange={postControllerState}
                    ></Slider>
                    <Slider 
                        label="Tolerance" 
                        bind:value={controllerState.tolerance}
                        min={0.04} 
                        max={0.4} 
                        step={0.01}
                        onChange={postControllerState}
                    ></Slider>
                </GridForm>
                <span class="text-lg">Limits</span>
                <GridForm>
                    <Checkbox 
                        label="Enable" 
                        bind:value={controllerState.limits.enabled}
                        onChange={postControllerState}
                    ></Checkbox>
                    {#if controllerState.limits.enabled}
                    <Slider 
                        label="Begin" 
                        bind:value={controllerState.limits.begin}
                        min={0} 
                        max={360} 
                        step={0.01}
                        onChange={postControllerState}
                    ></Slider>
                    <Slider 
                        label="End" 
                        bind:value={controllerState.limits.end}
                        min={0} 
                        max={360} 
                        step={0.01}
                        onChange={postControllerState}
                    ></Slider>
                    {/if}
                </GridForm>
                <!-- <span class="text-lg">Calibration</span>
                <GridForm>
                    <Checkbox 
                        label="Enable" 
                        bind:value={controllerState.calibration.enabled}
                        onChange={postControllerState}
                    ></Checkbox>
                    <Slider 
                        label="Speed" 
                        bind:value={controllerState.calibration.speed}
                        min={-20} 
                        max={20} 
                        step={1}
                        onChange={postControllerState}
                    ></Slider>
                    <Slider 
                        label="Decay" 
                        bind:value={controllerState.calibration.decay}
                        min={0.} 
                        max={0.2} 
                        step={0.01}
                        onChange={postControllerState}
                    ></Slider>
                </GridForm>
                <ChartComp
                    {label}
                    data={calibrationOffsets}
                ></ChartComp>
                <div class="flex flex-row flex-wrap justify-between gap-x-2">
                    <button class="btn btn-primary inline-flex items-center" 
                        on:click={startCalibration}
                        ><span>Start</span></button
                    >
                    <button class="btn btn-primary inline-flex items-center" 
                        on:click={stopCalibration}
                        ><span>Stop</span></button
                    >
                    <button class="btn btn-primary inline-flex items-center" 
                        on:click={resetCalibration}
                        ><span>Reset</span></button
                    >
                    <div class="flex-grow"></div>
                    <button class="btn btn-primary inline-flex items-center" 
                        on:click={getCalibrationOffsets}
                        ><span>Get Data</span></button
                    >
                    {#if calibrationOffsets?.length == controllerState.calibration.steps}
                    <button class="btn btn-primary inline-flex items-center" 
                        on:click={saveCalibration}
                        ><span>Save</span></button
                    >
                    {/if}
                </div> -->
            </Collapsible>
        </div>
    {/await}
    <div class="flex flex-row flex-wrap justify-between gap-x-2">
        <div class="flex-grow"></div>
        <div>
            <div>
                <DisableButton onClick={() => postJsonRest(restPath, {enabled: false, stepper: {control: {enabled: false}}})}></DisableButton>
                <StopButton onClick={() => postJsonRest(restPath, {enabled: false, stepper: {control: {stop: {}}}})}></StopButton>
            </div>
        </div>
    </div>
</SettingsCard>