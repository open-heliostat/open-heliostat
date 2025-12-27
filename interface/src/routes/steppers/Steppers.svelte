<script lang="ts">
	import { onMount, onDestroy } from 'svelte';
	import SettingsCard from '$lib/components/SettingsCard.svelte';
	import StepperRestComp from '$lib/components/StepperRestComp.svelte';

	onMount(() => {
	});

	onDestroy(() => {
	});

</script>

<SettingsCard>
	<span slot="title">Azimuth Stepper</span>
	<StepperRestComp restPath={"/rest/heliostat/azimuth/stepper"}></StepperRestComp>
</SettingsCard>
<SettingsCard>
	<span slot="title">Elevation Stepper</span>
	<StepperRestComp restPath={"/rest/heliostat/elevation/stepper"}></StepperRestComp>
</SettingsCard>