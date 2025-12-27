<script lang="ts">
	import { onMount, onDestroy } from 'svelte';
    import Collapsible from '$lib/components/Collapsible.svelte';
	import type { ControllerState, StepperDiag } from '$lib/types/models'
	import Slider from '$lib/components/Slider.svelte';
	import Checkbox from '$lib/components/Checkbox.svelte';
	import GridForm from '$lib/components/GridForm.svelte';
	import { user } from '$lib/stores/user';
	import { page } from '$app/stores';
	import { notifications } from '$lib/components/toasts/notifications';
	import Spinner from '$lib/components/Spinner.svelte';
	import StepperStatusComp from '$lib/components/StepperStatusComp.svelte';
	import ChartComp from '$lib/components/ChartComp.svelte';
	import SettingsCard from './SettingsCard.svelte';
	import StepperControlForm from './StepperControlForm.svelte';
	import StepperSettingsForm from './StepperSettingsForm.svelte';
	import StepperRestComp from './StepperRestComp.svelte';

    export let label: string;
    export let restPath : string;

    let controllerState : ControllerState;
    let calibrationOffsets : number[] | undefined;

    // onMount(() => {
    //     getControllerState();
    // });

    async function updateJsonRest<T>(path: string, destination : T) {
		try {
			const response = await fetch(path, {
				method: 'GET',
				headers: {
					Authorization: $page.data.features.security ? 'Bearer ' + $user.bearer_token : 'Basic',
					'Content-Type': 'application/json'
				}
			});
			let json = await response.json();
            destination = json;
		} catch (error) {
			console.error('Error:', error);
		}
		return destination;
	}

    async function postJsonRest<T>(path: string, data: T) {
		try {
			const response = await fetch(path, {
				method: 'POST',
				headers: {
					Authorization: $page.data.features.security ? 'Bearer ' + $user.bearer_token : 'Basic',
					'Content-Type': 'application/json'
				},
				body: JSON.stringify(data)
			});
			if (response.status == 200) {
				data = await response.json();
                console.log(data);
			} else {
				notifications.error('Wrong Path.', 3000);
			}
		} catch (error) {
			notifications.error('Error: ' + error, 3000);
		}
        // console.log(data);
        return data;
	}

    async function getControllerState() {
        const res = await fetch(restPath);
        const data = await res.json();
        controllerState = data;
        // console.log(data);
        return data;
        // return updateJsonRest(restPath, controllerState);
    }

    function postControllerState() {
        postJsonRest(restPath, controllerState).then((data) => controllerState = data);
    }

    function getCalibrationOffsets() {
        return updateJsonRest(restPath + '/calibration/offsets/', calibrationOffsets).then((data) => calibrationOffsets = data);
    }

    let calibrationIntervalID : any;

    function startCalibration() {
        if (controllerState) controllerState.calibration.running = true;
        calibrationIntervalID = setInterval(getCalibrationOffsets, 1011);
        postJsonRest(restPath + '/calibration/start', {});
    }
    
    function stopCalibration() {
        if (controllerState) controllerState.calibration.running = false;
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
            {:then data} 
                <StepperStatusComp stepperControl={data}></StepperStatusComp>
            {/await}
            <Slider 
                label="Position" 
                bind:value={controllerState.position}
                min={0} 
                max={360} 
                step={0.01}
                onChange={postControllerState}
            ></Slider>
            <span class="text-lg">Control</span>
            <GridForm>
                <Checkbox 
                    label="Enable" 
                    bind:value={controllerState.enabled}
                    onChange={postControllerState}
                ></Checkbox>
                <Slider 
                    label="Target" 
                    bind:value={controllerState.target}
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
        </div>
        <div>
            <span class="text-lg">Calibration</span>
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
            </div>
        </div>
    {/await}
    
    <!-- <Collapsible>
        <span slot="title">{label} Stepper</span>
        {#await fetch(restPath + '/stepper/control').then(async (res) => await res.json())}
        {:then data} 
            <StepperControlForm 
                stepperControl={data}
                onChange={()=>{}}
            ></StepperControlForm>
        {/await}
        {#await fetch(restPath + '/stepper/config').then(async (res) => await res.json())}
        {:then data} 
            <StepperSettingsForm 
                stepperSettings={data}
                onChange={()=>{}}
            ></StepperSettingsForm>
        {/await}
    </Collapsible> -->
    <!-- <Collapsible>
        <span slot="title">{label} Stepper</span>
        <StepperRestComp restPath={restPath + "/stepper"}></StepperRestComp>
    </Collapsible> -->

</SettingsCard>